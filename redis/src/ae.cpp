#include <exception>

/* Include the best multiplexing layer supported by this system.
The following should be ordered by performances in descending order.*/

#ifdef HAVE_EVPORT
#include "ae_evport.c"
#else
	#ifdef HAVE_EPOLL
	#include "ae_epoll.c"
	#else
		#ifdef HAVE_KQUEUE
		#include "ae_kqueue.c"
		#else
		#include "ae_select.c"
		#endif
	#endif
#endif

aeEventLoop aeCreateEventLoop(int setsize)
{
	try
	{
		aeEventLoop eventloop;
		eventloop.events = new aeFileEvent[setsize];
		eventloop.fired = new aeFiredEvent[setsize];
		eventloop.setsize = setsize;
		eventloop.lastTime = time(NULL);
		eventLoop.stop = 0;
		aeApiCreate(eventloop);
		/* Events with mask == AE_NONE are not set. So let's initialize the 
		vector with it. */
		for (int i = 0; i < setize; ++i)
			eventloop->events[i].mask = AE_NONE;
		return eventloop;
	}
	catch(exception& e)
	{
		cout << e.what() << endl;
	}
	catch(...)
	{
		cout << "aeCreateEventLoop new operation failed.\n";
	}
	return NULL;
}


int aeCreateFileEvent(aeEventLoop *eventloop, int fd, int mask,
	aeFileProc * proc, void * clientData)
{
	if (fd >= eventLoop->setsize)
	{
		errno = ERANGE;
		return AE_ERR;
	}
	aeFileEvent *fe = &eventloop->events[fd];

	if (aeApiAddEvent(eventloop, fd, mask) == -1)
		return AE_ERR;
	fe->mask |= mask;
	if (mask & AE_READABLE)
		fe->rfileProc = proc;
	if (mask & AE_WRITABLE)
		fe->wfileProc = proc;
	fe->clientData = clientData;
	if (fd > eventloop->maxfd)
		eventloop->maxfd = fd;
	return AE_OK;
 
}
