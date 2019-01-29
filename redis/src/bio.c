/* Background I/O service for Redis.

This file implements operations that we need to perform in the background.
Currently there is only a single operation, that is a background close(2)
system call. This is needed as when the process is the last owner of a
reference to a file, closing it means unlinking it, and the deletion of the
file is slow, blocking the server.

In the future, we will either continue implementing new things we need or
we'll switch to libeio. However, there are probably long term uses for this
file as we may want to put here Redis specific background tasks (for instance
it is not impossible that we'll need a non blocking FLUSHDB/FLUSHALL implementation.)

DESIGN
------

The design is trival, we have a structure representing a job to perform and a 
different thread and job queue for every job type. Every thread waits for new jobs
in its queue, and process every job sequentially.

Jobs of the same type are guaranteed to be processed from the least recently
inserted one to the most recently inserted.

Currently there is no way for the creator of the job to be notified about the 
completion of the operation, this will only be added when needed.  

*/

#include "bio.h"

/* Initialize the background system, spawning the thread. */
void Bio::init()
{
	pthread_attr_t attr;
	pthread_t thread;
	size_t stacksize;

	/* Initialization of state vars and objects */
	for (int j = 0; j < BIO_NUM_OPS; ++j)
	{
		pthread_mutex_init(&bio_mutex[j], NULL);
		pthread_cond_init(&bio_newjob_cond[j], NULL);
		pthread_cond_init(&bio_step_cond[j], NULL);
		bio_pending[j] = 0;
	}

	/* Set the stack size as by default it may be small in some system */
	pthread_attr_init(&attr);
	pthread_attr_getstacksize(&attr, &stacksize);
	if (!stacksize) stacksize = 1;
	while(stacksize < REDIS_THREAD_STACK_SIZE) 
		stacksize *= 2;
	pthread_attr_setstacksize(&attr, stacksize);
	
	/* Ready to spawn our threads. We use the single argument the thread
	function accepts in order to pass the job ID the thread is reponsible of.*/
	for (int j = 0; j < BIO_NUM_OPS; j++)
	{
		void *arg = (void*)(unsigned long) j;
		if (pthread_create(&thread, &attr, bioProcessBackgroundJobs, arg) != 0)
		{
			serverLog(LL_WARNING, "Fatal: Can't initialize Background Jobs.");
			exit(1);
		}
		bio_threads[j] = thread;
	}

}


void* biProcessBackgroundJobs(void *arg)
{
	struct bio_jobs *job;
	unsigned long type = (unsigned long) args;
	sigset_t sigset;

	/* check that the type is within the right interval */
	if (type >= BIO_NUM_OPS)
	{
		serverLog(LL_WARNING, "Warning: bio thread started with wrong type %lu", type);
	return NULL;
	}

	/* Make the thread killable at any time, so that bioKillThreads() can work reliably. */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	pthread_mutex_lock(&bio_mutex[type]);/* block until the mutex become available */
	/* Block SIGALRM so we are sure that only the main thread will receive the watchdog signal. */
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	if (pthread_sigmask(SIG_BLOCK, &sigset, NULL))
		serverLog(LL_WARNING, "Warning: can't mask SIGALRM in bio.c thread: %s", strerror(errno));
	while(1)
	{
		listNode * ln;
		/* The loop always starts with the lock hold*/
		if (listLength(bio_jobs[type]) == 0)
		{
			pthread_cond_wait(&bio_newjob_cond[type], &bio_mutex[type]);
			continue;
		}
		/* Pop the job from the queue */
		ln = listFirst(bio_jobs[type]);
		job = ln->value;
		/* It is now possible to unlock the background system as we know there
		is a stand alone job structure to process. */
		pthread_mutex_unlock(&bio_mutex[type]);

		/* Process the job accordingly to its type. */
		if (type == BIO_CLOSE_FILE)
			close((long)job->arg1);
		else if (type == BIO_AOF_FSYNC)
			redis_fsync((long)job->arg1);
		else if (type == BIO_LAZY_FREE)
		{
			/* What we free changes depending on what arguments are set:
			arg1 -> free the object at pointer.
			arg2 & arg3 -> free two dictionaries (a Redis DB). 
			only arg3 -> free the skiplist. */
			if (job->arg1)
				lazyfreeFreeObjectFromBioThread(job->arg1);
			else if (job->arg2 && job->arg3)
				lazyfreeFreeDatabaseFromBioThread(job->arg2, job->arg3);
			else if (job->arg3)
				lazyfreeFreeSlotsMapFromBioThread(job->arg3);
		}
		else
		{
			serverPanic("Wrong job type in bioProcessBackgroundJobs().");
		}	
		zfree(job);

		/* Lock again before reiterating the loop, if there are no longer jobs 
		to process, we'll block again in pthread_cond_wait(). */
		pthread_mutex_lock(&bio_mutex[type]);
		listDelNode(bio_jobs[type], ln);
		bio_pending[type]--;
		/* Unblock threads blocked on bioWaitStepOfType() if any. */
		pthread_cond_broadcast(&bio_step_cond[type]);	
	}
}
