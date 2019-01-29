#include <sys/epoll.h>

class aeApiState
{
	int epfd;
	struct epoll_event * events;
};

int aeApiAddEvent(aeEventLoop* eventloop, int fd, int mask)
{
	aeApiState *state = eventloop->apidata;
	struct epoll_event ee = {0};/* avoid valgrind warning */
	/* If the fd was already monitored for some event, we need
	a MOD operation. Otherwise, we need an ADD operation. */
	int op = eventloop->events[fd].mask == AE_NONE ?
		EPOLL_CTL_ADD : EPOLL_CTL_MOD;

	ee.events = 0;
	mask |= eventloop->events[fd].mask; /* Merge old events */
	if (mask & AE_READABLE)
		ee.events |= EPOLLIN;
	if (mask & AE_WRITABLE)
		ee.events |= EPOLLOUT;
	ee.data.fd = fd;
	if (epoll_ctl(state->epfd, op, fd, &ee) == -1)
		return -1;
	return 0;
}

int aeApiCreate(aeEventLoop& eventloop)
{
	aeApiState *state = new aeApiState();
	if (!state) 
		return -1;
	state->events = zmalloc(sizeof(struct epoll_event) * eventloop.setsize);
	if (!state->events)
	{
		zfree(state);
		return -1;
	}
	state->epfd = epoll_create(1024); /* 1024 is just a hint for the kernel, #define __FD_SETSIZE 1024 in posix_types.h for select*/
	if (state->epfd == -1)
	{
		zfree(state->events);
		delete state;
		return -1;
	}
	eventloop.apidata = state;
	return 0;
}
