
/* Background job opcodes */
#define BIO_NUM_OPS 3

class bio_job
{
	time_t time;/* Time the job was created. */
	/*Job specific arguments pointers. If we need to pass m ore than
	three arguments, we can just pass a pointer to a structure. */
	void *arg1, *arg2, *arg3;
}

class Bio
{
	pthread_mutex_t bio_mutex[BIO_NUM_OPS];
	pthread_cond_t bio_newjob_cond[BIO_NUM_OPS];
	pthread_cond_t bio_step_cond[BIO_NUM_OPS];
	list<bio_jobs> jobs[BIO_NUM_OPS];
	/* The following array is used to hold the number of pending jobs for every
	OP type. This allows us to export the bioPendingJobsOfType() API that is useful
	when the main thread wants to perform some opertion that may involve objects
	shared with the background thread. The main thread will just wait that there
	are no longer jobs of this type to be executed before performing the sensible
 	operation. This data is alos useful for reporting.*/
	unsigned long long bio_pending[BIO_NUM_OPS];
public:
	init();
};
