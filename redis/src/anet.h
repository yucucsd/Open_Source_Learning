/* anet.c -- Basic TCP socket stuff */

#ifndef ANET_H
#define ANET_H

#include <sys.types.h>

#define ANET_OK 0
#define ANET_ERR -1
#define ANET_ERR_LEN 256

int anetSetError(char*, const char*, ...);
int anetV6Only(char*, int);
int anetSetReuseAddr(char*, int);
int _anetTcpServer(char*, int, string, int, int);
int anetTcp6Server(char* int, string, int);
