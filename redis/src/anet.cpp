#include <sys/socket.h> /* for addrinfo */
#include <netdb.h> /* for getaddrinfo */
#include <stdarg.h> /* for va_list */
void anetSetError(char* err, const char *fmt, ...)
{
	va_list ap;

	if (!err) return;
	va_start(ap, fmt);
	vsnprintf(err, ANET_ERR_LEN, fmt, ap);
	va_end(ap);
}

int anetSetBlock(char *err, int fd, int non_block)
{
	int flags;
	/* Set the socket blocking (if non_block is zero) or non blocking.
	Note that fcntl(2) for F_GETFL and F_SETFL can't be 
	interrupted by a signal. */
	if ((flags = fcntl(fd, F_GETFL)) == -1)
	{
		anetSetError(err, "fcntl(F_GETFL): %s", strerror(errno));
		return ANET_ERR;
	}
	
	if (non_block)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	if (fcntl(fd, F_SETFL, flags) == -1)
	{
		anetSetError(err, "fcntl(F_SETFL, O_NONBLOCK): %s", strerror(errno));
		return ANET_ERR;
	}
	return ANET_OK;
}

int anetV6Only(char *err, int s)
{
	int yes = 1;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes)) == -1)
	{
		anetSetError(err, "setsockopt: %s", strerror(errno));
		close(s);
		return ANET_ERR;
	}
	return ANET_OK;
}

int anetNonBlock(char *err, int fd)
{
	return anetSetBlock(err, fd, 1);
}

int anetSetReuseAddr(char* err, int fd)
{
	int yes = 1;
	/* Make sure connection-intensive things like the redis benchmark
	 will be able to close/open sockets a zillion of times. 
	Usually, a socket could only be reused after being released for 2 minutes,
	use SO_REUSEADDR to make immediate reuse possible. */
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
	{
		anetSetError(err, "setsockopt SO_REUSEADDR: %s", strerror(errno));
		return ANET_ERR;
	}
	return ANET_OK;
}

int anetListen(char* err, int s, struct sockaddr *sa, socklen_t len, int backlog)
{
	if (bind(s, sa, len) == -1)
	{
		anetSetError(err, "bind: %s", strerror(errno));
		close(s);
		return ANET_ERR;
	}
	if (listen(s, backlog) == -1)
	{
		anetSetError(err, "bind: %s", strerror(errno));
		close(s);
		return ANET_ERR;
	}
	return ANET_OK;
}

void setError(int& s)
{
	if (s != -1)
		close(s);
	s = ANET_ERR; 
}

int _anetTcpServer(char* err, int port, char* bindaddr, int af, int backlog)
{
	int s = -1, rv;
	char _port[6];
	struct addrinfo hints, *servinfo, *p;
	
	/* copy from source to destination with length (n - 1) and append a 0 */
	snprintf(_port, sizeof(_port), "%d", port); 
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = af;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((rv = getaddrinfo(bindaddr, _port, &hints, &servinfo)) != 0 )
	{
		anetSetError(err, "%s", gai_strerror(rv));
		return ANET_ERR;
	} 
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;
		if (af == AF_INET6 && anetV6Only(err, s) == ANET_ERR)
			setError(s);
		if (anetSetReuseAddr(err, s) == ANET_ERR)
			setError(s);
		if (anetListen(err, s, p->ai_addr, p->ai_addrlen, backlog) == ANET_ERR)	
			s = ANET_ERR;
		freeaddrinfo(servinfo);
		return s;
	}
	if (p == NULL)
	{
		anetSetError(err, "unable to bind socket, errno: %d", errno);
		freeaddrinfo(servinfo);
		return s;
	}
}
int anetTcpServer(char* err, int port, char* bindaddr, int backlog)
{
	return _anetTcpServer(err, port, bindaddr, AF_INET, backlog);
}
int anetTcp6Server(char* err, int port, char* bindaddr, int backlog)
{
	return _anetTcpServer(err, port, bindaddr, AF_INET6, backlog);
}
