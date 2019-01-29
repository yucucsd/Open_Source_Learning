#include "redisserver.h"

/* Low level logging. */
void redisServer::logRaw(int level, const char *msg)
{
	const int sysloglevel[] = {LOG_DEBUG, LOG_INFO, LOG_NOTICE, LOF_WARNING};
	const string c = ".-*#";
	FILE *fp;
	char buf[64];
	int rawmode = (level & LL_RAW);
	int log_to_stdout = logfile.size() == 0;
	
	level &= 0xff;
	if (level < verbosity) 
		return;

	fp = log_to_stdout ? stdout : fopen(logfile, "a");
	if (!fp) return;
	
	if (rawmode)
		fprintf(fp, "%s", msg);
	else
	{
		int off;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		struct tm tm;
		nolocks_localtime(&tm, tv.tv_sec, timezone, daylight_active);
		off = strftime(buf, sizeof(buf), "%d %b %Y %H:%M:%S", &tm);	
		snprintf(buf+off, sizeof(buf) - off, "%03d", (int)tv.tv_usec/1000);
		fprintf(fp, "%d %s %c %s \n", (int)getpid(), buf, c[level], msg);
	}
	fflush(fp);

	if (!log_to_stdout) fclose(fp);
	if (syslog_enabled) syslog(syslogLevelMap[level], "%s", msg);
}

/* Like logRaw() but with printf-alike support. This is the function that 
is used across the code. The raw version is only used in order to dump
the INFO output on crash.
*/
void redisServer::log(int level, const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_LEN];
	if ((level&0xff) < verbosity) return;

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	logRaw(level, msg);
}

void redisServer::panic(const char* file, int line, const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	char fmtmsg[256];
	vsnprintf(fmtmsg, sizeof(fmtmsg), msg, ap);
	va_end(ap);

	bugReportStart();
	log(LL_WARNING, "---------------------------------------------");
	log(LL_WARNING, "!!! Software Failure. Press left mouse button to continue");
	log(LL_WARNING, "Guru Mediatation: %s #%s:%d", fmtmsg, file, line);
#ifdef HAVE_BACKTRACE
	log(LL_WARNING, "forcing SIGSEGV in order to print the stack trace");
#endif
	log(LL_WARNING, "---------------------------------------------");
	*((char*)-1) = 'x'; /* Segmentation fault? */
	
}

void redisServer::bugReportStart()
{
	if (bug_report_start == 0)
	{
		logRaw(LL_WARNING|LL_RAW,
			"\n\n=== REDIS BUG REPORT START: Cut & paste starting from here ===\n");
		bug_report_start = 1;
	}
}

/* We store a cached value of the unix time in the global state because
when accuracy is not needed, with virtual memory and aging, at every object 
access, accessing a global var is a lot faster than calling time(NULL). */ 
void updateCachedTime()
{
	time_t unixtime_l = time(NULL);
	atomicSet(unixtime, unixtime_l);
	mstime = mstime();

	/* To get information about daylight saving time, we need to call
	localtime_r and cache the result. However calling localtime_r in this
	context is safe since we will never fork() while here, in the main thread.
	The logging function will call a thread safe version of localtime
	that has no locks. */
	struct tm tm;
	localtime_r(unixtime, &tm);
	daylight_active = tm.tm_isdst;

}

/* Initialize a set of file descriptors to listen to the specified
'port' binding the addresses specified in the Redis server configuration.

The listening file descriptors are stored in the integer array 'fds' and
their number is set in count.

The addresses to bind are specified in the global server.bindaddr array
and their number is server.bindaddr_count. If the server configuration 
contains no specific addresses to bind, this function will try to bind *
(all addresses) for both IPv4 and IPv6 protocols.

On success the function returns C_OK.

On error the function returns C_ERR. For the function to be on error, at
least one of the server.bindaddr addresses was impossible to bind, or no 
bind addresses were specified in the server configuration but the function
is not able to bind * for at least one of the IPv4 or IPv6 protocols. */

int redisServer::listenToPort(int port, int *fds, int& count)
{
	int j;
	/* Force binding of 0.0.0.0 (IP address of localhost) if no bind 
	address is specified, always entering the loop if j == 0. */
	if (bindaddr_count == 0) 
		bindaddr[0] = NULL;
	for (j = 0; j < bindaddr_count || j == 0; j++)
	{
		if (bindaddr[j] == NULL)
		{
			int unsupported = 0;
			/* Bind * for both IPv6 and IPv4, we enter here only
			if bindaddr_count == 0. */
			fds[count] = anetTcp6Server(neterr, port, NULL, tcp_backlog);
			if (fds[count] != ANET_ERR)
			{
				anetNonBlock(NULL, fds[count]);
				++count;
			}
			else if (errno == EAFNOSUPPORT)
			{
				++unsupported;
				log(LL_WARNING, "Not listening to IPv6: unsupported");
			}

			if (count == 1 || unsupported)
			{
				/* Bind the IPv4 address as well. */
				fds[count] = anetTcpServer(neterr, port, NULL, tcp_backlog);
				if (fds[count] != ANET_ERR)
				{
					anetNonBlock(NULL, fds[count]);
					++count;
				}
				else if (errno == EAFSUPPORT)
				{
					++unsupported;
					serverLog(LL_WARNING, "Not listening to IPv4: unsupported");
				}
			}
			
			/* Exit the loop if we were able to bind * on IPv4 and IPv6,
			otherwise, fds[count] will be ANET_ERR and we'll print an
			error and return to the caller with an error. */
			if (count + unsupported == 2)
				break;
		}
		else if (strchr(bindaddr[j], ':'))
		{
			/* Bind IPv6 address. */
			fds[count] = anetTcp6Server(neterr, port, bindaddr[j], tcp_backlog);
		}
		else
		{
			/* Bind IPv4 address. */
			fds[count] = anetTcpServer(neterr, port, bindaddr[j], tcp_backlog);
		}
		if (fds[count] == ANET_ERR)
		{
			log(LL_WARNING, "Creating Server TCP listening socket %s:%d: %s",
				bindaddr[j] ? bindaddr[j] : "*", port, neterr);
			return C_ERR;
		}
		anetNonBlock(NULL, fds[count]);
		++count;
	}
	return C_OK;
}

void redisServer::init()
{
	int j;

	hz = config_hz;
	pid = getpid();
	current_client = NULL;
	clients_index = raxNew();
	slavesldb = -1; /* Force to emit the first SELECT command. */
	get_ack_from_slaves = 0;
	clients_paused = 0;
	el = aeCreateEventLoop(maxclients + CONFIG_FDSET_INCR);
	/* Open TCP listening socket for the user commands. */
	if (port != 0
		&& listenToPort(port, ipfd, ipfd_count) == C_ERR)
		exit(1);
	 
	/* Open the listening Unix domain socket. */
	if (unixsocket != NULL)
	{
		
	} 

	if (ipfd_count == 0 && sofd < 0)
	{
		log(LL_WARNING, "Configured to not listen anywhere, exiting.");
		exit(1);
	}
	
	/* Create the Redis databases, and initialize other internal state. */
	for (j = 0; j < dbnum; ++j)
	{
		db[j].dict = dictCreate(&dbDickType, NULL);
		db[j].expires = dictCreate(&keyptrDictType, NULL);
		db[j].blocking_keys = dictCreate(&keylistDictType, NULL);
		db[j].ready_keys = dictCreate(&objectKeyPointerValueDictType, NULL);
		db[j].watched_keys = dictCreate(&keylistDictType, NULL);
		db[j].id = j;
		db[j].avg_ttl = 0;
		db[j].defrag_later = listCreate(); 
	}
	evictionPoolAlloc();
	pubsub_channels = dictCreate(&keylistDictType, NULL);
	pubsub_patterns = listCreate();
	
	/* Create an event handler for accepting new connections in TCP and Unix 
	domain sockets. */
	for (j = 0; j < ipfd_count; ++j)
	{
		if (aeCreateFileEvent(el, ipfd[j], AE_READABLE, 
			acceptTcpHandler, NULL) == AE_ERR)
			panic(__FILE__, __LINE__, "Uncoverable error creating ipfd file event.");
	}
	if (sofd > 0 && aeCreateFileEvent(sl, sofd, AE_READABLE, acceptUnixHandler, 
		NULL) == AE_ERR)
		panic(__FILE__, __LINE__, "Uncoverable error creating server sofd file event.");	
	
	/* Register a readable event for the pipe used to awake the event loop
	when a blocked client in a module needs attention. */
	if (aeCreateFileEvent(server.el, server.module_blocked_pipe[0], AE_READABLE,
		moduleBlockedClientPipeReadable, NULL) == AE_ERR)
		panic("Error registering the readable event for module blocked clients subsystem");

	/* 32 bit instances are limited to 4GB of address space, so if there is no explicit limit
	in the user provided configuration, we set a limit at 3GB using maxmemory with "noeviction" 
	policy. This avoids useless crashes of the Redis instance for out of memory.*/
	if (arch_bits == 32 && maxmemory == 0)
	{
		log(LL_WARNING, "Warning: 32 bit instance detected but no memory limit set."
			"Setting 3GB maxmemory limit with 'noeviction' policy now.")
		maxmemory = 3072LL * (1024 * 1024);
		maxmemory_policy = MAXMEMORY_NO_EVICTION;
	}
	bioInit();
	server.initial_memory_usage = zmalloc_used_memory();
}

void redisServer::initConfig(void)
{
	int j;
	updateCachedTime();
	configfile = "";
	executable = "";
	ipfd_count = 0;
	sofd = -1;
	arch_bits = (sizeof(long) == 8) ? 64 : 32;
	port = CONFIG_DEFAULT_SERVER_PORT;
	backlog = CONFIG_DEFAULT_TCP_BACKLOG;
	verbosity = CONFIG_DEFAULT_VERBOSITY;
	logfile = CONFIG_DEFAULT_LOGFILE;
	timezone = getTimeZone();
	syslog_enabled = CONFIG_DEFAULT_SYSLOG_ENABLED;
	maxmemory = CONFIG_DEFAULT_MAXMEMORY;
	maxmemory_policy = CONFIG_DETAULT_MAXMEMORY_POLICY;	
}

void redisServer::loadConfig(string cf, string opts)
{

}

#ifdef __linux__
int linuxOvercommitMemoryValue(void)
{
	FILE *fp = fopen("/proc/sys/vm/overcommit_memory", "r");
	char buf[64];

	if (!fp) 
		return -1;
	if (fgets(buf, 64, fp) == NULL)
	{
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return atoi(buf);
}

void linuxMemoryWarning(void)
{
	if (linuxOvercommitMemoryValue() == 0)
	{
		serverLog(LL_WARNING, "WARNING overcommit_memory is set to 0! Background save may fail under low memory condition. To fix"
			" this issue, add 'vm.overcommit_memory = 1' to /etc/sysctl.conf and then reboot or run the"
			" command 'sysctl vm.overcommit_memory = 1' for this to take effect. ");
	}
	if (THPIsEnable())
	{
		serverLog(LL_WARNING, "WARNING you have Transparent Huge Pages (THP) support enable in your kernel. This will latency and"
			" memory usage issue with Redis. To fix this issue, run the "
			" command 'echo never > /sys/kernel/mm/transparent_hugepage/enabled' as root, and add it to your '/etc/rc.local'"
			" in order to retain the setting after a reboot. Redis must be restarted after THP is disabled.");
	}
}
#endif

/* Check that server.tcp_backlog can be actually enforced in Linux according
to the value of /proc/sys/net/core/somaxconn, or warn about it. */
void checkTcpBacklogSettings(void)
{
#ifdef HAVE_PROC_SOMAXCONN
	FILE *fp = fopen("/proc/sys/net/core/somaxconn", "r"); //somaxconn defines the maximum length of backlog queue
	char buf[1024];
	if (!fp) return;
	if (fgets(buf, sizeof(buf), fp) != NULL)
	{
		int somaxconn = atoi(buf);
		if (somaxconn > 0 && somaxconn < server.tcp_backlog)
		{
			serverLog(LL_WARNING, "WARNING: The TCP backlog setting of %d cannot be enforced because /proc/sys/net/core/somaxconn is set to the lower value of %d", server.tcp_backlog, somaxconn);
		}
	}
}

redisServer server;

void version(void)
{
	cout <<  "Redis server v =" << REDIS_VERSION << endl;
}

void usage(void)
{
	fprintf(stderr, "Usage: ./redis-server [/path/to/redis.conf] [options]\n");
	fprintf(stderr, "       ./redis-server - (read config from stdin)\n");
	fprintf(stderr, "       ./redis-server -v or --version\n");
	fprintf(stderr, "       ./redis-server -h or --help\n");
	fprintf(stderr, "       ./redis-server --test-memory <megabytes>\n\n");
	fprintf(stderr, "Examples:\n");
	fprintf(stderr, "       ./redis-server (run the server with default conf)");
	fprintf(stderr, "       ./redis-server /etc/redis/6379.conf\n");
	fprintf(stderr, "       ./redis-server --port 7777\n");
	fprintf(stderr, "       ./redis-server --port 7777 --replicaof 127.0.0.1 8888\n");
	fprintf(stderr, "       ./redis-server /etc/myredis.conf --loglevel verbose\n\n");
	fprintf(stderr, "Sentinel mode:\n");
	fprintf(stderr, "       ./redis-server /etc/sentinel.conf --sentinel\n");
	exit(1);
	
}

void memtest(size_t megabytes, int passes);

int main(int argc, char* argv[])
{
	tzset();
	
	char hashseed[16];
	getRandomHexChars(hashseed, sizeof(hashseed));
	dictSetHashFunctionSeed((uint8_t*)hashseed);
	server.initConfig();
	server.executable = getAbsolutePath(argv[0]);
	for (int i = 0; i < argc; ++i)
		server.exec_argv.push_back(string(argv[i]));
	if (argc >= 2)
	{
		string configfile;
		if (strcmp(argv[1], "-v") == 0
			|| strcmp(argv[1], "--version") == 0)
		{
			version();
		}
		if (strcmp(argv[1], "-h") == 0
			||strcmp(argv[1], "--help") == 0)
		{
			usage();
		}
		if (strcmp(argv[1], "--test-memory") == 0)
		{
			if (argc == 3)
			{
				memtest(atoi(argv[2]), 10);
				exit(0);
			}
			else
			{
				fprintf(stderr, "Please specify the amount of memory to test in megabytes.\n");
				fprintf(stderr, "Example: ./redis-server --test-memory 4096\n\n");
				exit(1);
			}
		}
		
		/* First argument is the config file name? */
		if (strlen(argv[1]) > 1 && (argv[1][0] != '-' || argv[1][1] != '-'))
		{
			configfile = string(argv[1]);
			server.configfile = getAbsolutePath(configfile);
			/* Replace the config file in server.exec_argv with its absolute path */ 
			server.exec_argv[1] = server.configfile;
		}

		int j = 2;
		string options = "";
		/* Append all other options to a single string and append it to configuration file */ 
		while (j != argc)
		{
			if (server.exec_argv[j][0] == '-' && server.exec_argv[j][1] == '-')
			{
				/* Option name */
				if (!server.exec_argv[j].compare("--check-rdb"))
				{
					/* argument has no options, skip for parsing. */
					++j; continue;
				}
				if (options.length() > 0) 
					options += "\n";
				options += string(server.exec_argv[j], 2);
				options += " ";
			}
			else
			{	
				/* Option argument */
				options += server.exec_argv[j];
				options += " ";
			}
			++j;
		}
		server.loadConfig(configfile, options);
	}
	if (argc == 1)
	{
		cout << "Warning: no config file specified, usin ghte default config." << endl;
	}
	else
	{
		cout << "Configuration loaded." << endl;
	}
	initServer();
	checkTcpBacklogSetting();
	
	if (!server.serntinel_mode)
	{
		/* Things not needed when running in Sentinel mode */
		serverLog(LL_WARNING, "Server initialized");
	#ifdef __linux__
		linuxMemoryWarnings();
	#endif
		moduleLoadFromQueue();
	}
}
