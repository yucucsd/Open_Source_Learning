/* Log levels */
#define LL_DEBUG 0
#define LL_VERBOSE 1
#define LL_NOTICE 2
#define LL_WARNING 3
#define LL_RAW (1<<10)
#define CONFIG_DEFAULT_VERBOSITY LL_NOTICE


/* Error Codes */
#define C_OK 0
#define C_ERR -1

/* Redis maxmemory strategies. Instead of using just incremental number,
we use a set of flags so that testing for certain properties common to
multiple policies is faster. */
#define MAXMEMORY_FLAG_LRU (1 << 0) 
#define MAXMEMORY_FLAG_LFU (1 << 1)
#define MAXMEMORY_FLAG_ALLKEYS (1 << 2)
#define MAXMEMORY_FLAG_NO_SHARED_INTEGERS (MAXMEMORY_FLAG_LRU | MAMEMORY_FLAG_LFU)

#define MAXMEMORY_VOLATILE_LRU ((0 << 8) | MAXMEMORY_FALG_LRU)
#define MAXMEMORY_VOLATILE_LFU ((1 << 8) | MAXMEMORY_FLAG_LFU)
#define MAXMEMORY_VOLATILE_TTL (2 << 8)
#define MAXMEMORY_VOLATILE_RANDOM (3 << 8)
#define MAXMEMORY_ALLKEYS_LRU ((4 << 8) | MAXMEMORY_FLAG_LRU | MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_ALLKEYS_LFU ((5 << 8) | MAXMEMORY_FLAG_LFU | MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_ALLKEYS_RANDOM ((6 << 8) | MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_NO_EVICTION (7 << 8)
#define CONFIG_DEFAULT_MAXMEMORY_POLICY MAXMEMORY_NO_EVICTION

/* Static Server configuration */
#define CONFIG_BINDADDR_MAX 16
#define CONFIG_DEFAULT_LOGFILE ""
#define CONFIG_DETAULT_MAXMEMORY_POLICY MAXMEMORY_NO_EVICTION
#define CONFIG_DEFAULT_SERVER_PORT 6379 /* TCP port */
#define CONFIG_DEFAULT_SYSLOG_ENABLED 0
#define CONFIG_DEFAULT_TCP_BACKLOG 511  /* TCP listening backlog */
#define CONFIG_MIN_RESERVED_FDS 32
#define LOG_MAX_LEN 1024                /* Default maximum length of syslog messages. */
#define NET_IP_STR_LEN 46               /* INET6_ADDRSTRLEN is 46 */

/* Protocol and I/O related defines */
#define PROTO_REPLY_CHUNK_BYTES (16 * 1024) /* 16k output buffer. */


/* When configuring the server eventloop, we setup it so that the total number 
of file descriptors we can handle are maxclients + RESERVED_FDS + a few more to
stay safe. Since RESERVED_FDS defaults to 32, we add 96 in order to make sure of
not over provisioning more than 128 fds. */
#define CONFIG_FDSET_INCR (CONFIG_MIN_RESERVED_FDS + 96)

/* Byte ordering detection */
#include <sys/types.h>	/*This will likely define BYTE_ORDER*/

#ifndef BYTE_ORDER
	#if (BSD > 199103)
		#include <machine/endian.h>
	#else
		#if defined (linux) || defined(__linux__)
			#include <endian.h>
		#else
			#define LITTLE_ENDIAN 1234	/* least significant byte first (vax, pc) */
			#define BIG_ENDIAN 4321		/* most significant byte first (IBM, net) */
			#define PDP_ENDIAN 3412		/* LSB first in word, MSW first in long (pdp) */

			#if defined(__i386__) || defined(__x86_64__) || defined(__amd64__) || \
   				defined(vax) || defined(ns32000) || defined(sun386) || \
   				defined(MIPSEL) || defined(_MIPSEL) || defined(BIT_ZERO_ON_RIGHT) || \
   				defined(__alpha__) || defined(__alpha)
				
				#define BYTE_ORDER LITTLE_ENDIAN
			#endif

			#if defined(sel) || defined(pyr) || defined(mc68000) || defined(sparc) || \
    			defined(is68k) || defined(tahoe) || defined(ibm032) || defined(ibm370) || \
    			defined(MIPSEB) || defined(_MIPSEB) || defined(_IBMR2) || defined(DGUX) ||\
    			defined(apollo) || defined(__convex__) || defined(_CRAY) || \
    			defined(__hppa) || defined(__hp9000) || \
    			defined(__hp9000s300) || defined(__hp9000s700) || \
			    defined (BIT_ZERO_ON_LEFT) || defined(m68k) || defined(__sparc)

				#define BYTE_ORDER BIG_ENDIAN
			#endif
		#endif	/* linux */
	#endif	/* BSD */
#endif	/* BYTE_ORDER */

/* Sometimes after including an OS-specific header that defines the 
endianess we end with __BYTE_ORDER but not with BYTE_ORDER that is what
the Redis code uses. In this case, let's define everything without the underscores. */
#ifndef BYTE_ORDER
	#ifdef __BYTE_ORDER
		#if defined (__LITTLE_ENDIAN) && defined(__BIG_ENDIAN)
			#ifndef LITTLE_ENDIAN && defined(__BIG_ENDIAN)
				#ifndef LITTLE_ENDIAN
					#define LITTLE_ENDIAN __LITTLE_ENDIAN
			    #endif
			    #ifndef BIG_ENDIAN
			    	#define BIG_ENDIAN __BIG_ENDIAN
			    #endif
			    #if (__BYTE_ORDER == __LITTLE_ENDIAN)
			    	#define BYTE_ORDER LITTLE_ENDIAN
			    #else
			    	#define BYTE_ORDER BIG_ENDIAN
			    #endif
			#endif
		#endif
	#endif
#endif

#if !defined(BYTE_ORDER)
	(BYTE_ORDER != BIG_ENDIAN && BYTE_ORDER != LITTLE_ENDIAN)
	/* You must determine what the correct bit order is for your 
	compiler - the next line is an intentional error which will
	force your compiler bomb until you fix the above macros. */
#error "Undefined or invalid BYTE_ORDER"
#endif

