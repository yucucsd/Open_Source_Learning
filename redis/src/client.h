#include "config.h"
class client
{
	unsigned long long id; /* Client increemental unique ID. */
	int fd;      /* Client socket. */
	std::string querybuf; /* Buffer we use to accumulate client queries. */

	/* Response buffer */
	int bufpos;
	char buf[PROTO_REPLY_CHUNK_BYTES];
	
};
