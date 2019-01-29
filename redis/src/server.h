#ifndef __REDIS_H
#define  __REDIS_H
#include <string>
#include <cstdlib>
#include <list>
#include "client.h"
#include "dict.h"
using namespace std;

struct moduleLoadQueueEntry
{
	sds path;
	int argc;
	robj **argv;
};

class redisDb 
{
	dict dictionary;     /* The keyspace for this DB. */
	dict expires;        /* Timeout of keys with a timeout set. */
	dict blocking_keys;  /* Keys with clients waiting for data. */
	dict ready_keys;     /* Blocked keys that received a PUSH. */
	dict watched_keys;   /* WATCHED keys for MULTI/EXEC CAS. */
	int id;              /* Database ID. */
	long long avg_ttl;   /* Average TTL, just for stats. */
	//list ;               /* list of key names to attempt to defrag one by one, gradually. */

};

class redisServer
{
	int hz;
	pid_t pid;
	Bio bio; /* Background I/O service */	

public:
	/* General */
	string configfile;	  /* Absolute config file path or NULL. */
	vector<string> exec_argv; /* Executable argv vector (copy). */
	string executable;        /* Absolute executable file path. */
	vector<aeEventLoop> el;	  /* event list */
	int arch_bits;            /* 32 or 64 depending on sizeof(long) */
	/* Networking */
	int port;                            /* TCP listening port */
	string bindaddr[CONFIG_BINDADDR_MAX];/* Addresses we should bind to. */
	int bindaddr_count;                  /* Number of addresses in bindaddr[] */
	int ipfd[CONFIG_BINDADDR_MAX];       /* TCP socket descriptors */
	int ipfd_count;                      /* Used slots in ipfd[] */
	int sofd;			     /* Unix socket file descriptor */
	string neterr;                       /* Error buffer for anet.c */
	int tcp_backlop;                     /* TCP listen() backlog */	

	list<client> clients;
	list<client> clients_to_close;
	client current_client;

	/* Logging */
	int verbosity;			     /* Loglevel in redis.conf */
	string logfile;                      /* Path of log file */
	
	/* time cache */
	time_t unixtime;                     /* Unix time sampled every cron cycle. */
	time_t timezone;		     /* Cached timezone. As set by tzset(). */
	int daylight_active;                 /* Currently in daylight saving time. */

	/* Pubsub */
	list<dict> pubsub_channels;		     /* Map channels to list of subscribe clients */
	list pubsub_patterns;		     /* A list of pubsub_patterns */
	/* Mutexes used to protect atomic variables when atomic builtins are 
	not available. */
	pthread_mutex_t unixtime_mutex;

	/* Member functions */
	void init();
	void loadConfig(string, string);
	void initConfig();
	void logRaw(int, const char*);
	void log();
	void updateCachedTime();
	int listenToPort(int, int*, int);
	void panic();
};
#endif
