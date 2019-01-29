
void acceptTcpHandler(aeEventLoop *el, int fd, void* privdata, int mask)
{
	int cport, cfd, max = MAX_ACCEPTS_PER_CALL;
	char cip[NET_IP_STR_LEN];
}
