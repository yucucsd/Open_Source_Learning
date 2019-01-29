/* A simple event-driven programming library. */

#define AE_OK 0
#define AE_ERR -1
#define AE_NONE 0         /* No events registered. */
#define AE_READABLE 1
#define AE_WRITABLE 2
#define AE_BARRIER 4


class aeEventLoop
{
	int maxfd;
	int setsize;
	long long timeEventNextId;
	time_t lastTime;
	aeFiredEvent * e
	vector<aeFileEvent> events;
	aeApiState* apidata; /* This is used for polling API specific data */
}
