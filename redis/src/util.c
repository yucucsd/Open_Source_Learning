#include "util.h"
#include <unistd.h>
#include <iostream>
#include <sys/time.h> /* for testing efficiency */
using namespace std;
/* using c++ string instead of Redis sds may cause efficiency problem. */

bool could_simplify_path(string fn)
{
	return (string(fn, 0, 3).compare("../") == 0)
		|| (fn[0] == '.' && fn[1] == '/');
}
/* Get random bytes, attempts to get an initial seed from /dev/urandom and 
then use a one way hash function in counter mode to generate a random stream.
However, if /dev/urandom is not available, a weaker seed is used.

This function is not thread safe, since the state is global. */
void getRandomBytes(unsigned char *p, size_t len)
{
	/* Global state. */
	static int seed_initialized = 0;
	static unsigned char seed[20]; /* The SHA1 seed, from /dev/urandom. */
	static uint64_t counter = 0;   /* The counter we hash with the seed.*/

	if (!seed_initialized)
	{
		/* Initialized a seed and use SHA1 in counter mode, where we hash 
		the same seed with a progressive counter. For the goals of this
		function we just need non-colliding strings, there are no 
		cryptographic security needs. */
		FILE *fp = fopen("/dev/urandom", "r");
		if (fp == NULL || fread(seed, sizeof(seed), 1, fp) != 1)
		{
			/* Revert to a weaker seed, and in this case reseed again 
			at every call. */
			for (unsigned int j = 0; j < sizeof(seed); j++)
			{
				struct timeval tv;
				gettimeofday(&tv, NULL);
				pid_t pid = getpid();
				seed[j] = tv.tv_sec ^ tv.tv_usec ^ pid ^ (long)fp;
			}

		}
		else
		{
			seed_initialized = 1;
		}
		if (fp)
			fclose(fp);
	}

	while(len)
	{
		unsigned char digest[20];
		SHA1_CTX ctx;
		unsigned int copylen = len > 20 ? 20 : len;

		SHA1Init(&ctx);
		SHA1Update(&ctx, seed, sizeof(seed));
		SHA1Update(&ctx, (unsigned char*)&counter, sizeof(counter));
		SHA1Final(digest, &ctx);
		counter++;

		memcpy(p, digest, copylen);
		len -= copylen;
		p += copylen;
	}
}
/* Generate the Redis "Run ID", a SHA1-sized random number that identifies 
a given execution of Redis, so that if you are talking with an instance
having run_id == A, and you reconnect and it  has run_id == B, you can be 
sure that it is either a different instance or it was restarted.*/
void getRandomHexChars(char *p, size_t len)
{
	char* charset = "0123456789abcdef";
	size_t j;

	getRandomBytes((unsigned char*)p, len);
	for (j = 0; j < len; j++)
		p[j] = charset[p[j] & 0x0F];
}
/* Given the filename, return the absolute path as an SDS string, or NULL
if it fails for some reason. Note that "filename" may be an absolute
path already, this will be detected and handled correctly.

The function does not try to normalize everything, but only the obvious
case of one or more "../" appearing at the start of "filename" relative
path. */
string getAbsolutePath(string filename)
{
	if (filename[0] == '/')
		return filename;
	/* join current working directory and relative path*/
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
		return "";
	string abspath(cwd);
	/* The while loop needs efficency testing. */
	while (filename.size() >= 3 && could_simplify_path(filename))
	{
		if (string(filename, 0, 3).compare("../") == 0)
		{
			size_t p = abspath.find_last_of ('/');
			if (p != string::npos)
				abspath.erase(p);
			filename.assign(filename, 3, string::npos);
		}		
		if (filename[0] == '.' && filename[1] == '/')
		{	
			filename.assign(filename, 2, string::npos);
		}
	}
	abspath += "/" + filename;
	return abspath;
}

unsigned long getTimeZone()
{
#ifdef __linux__
	return timezone;
#else
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);

	return tz.tz_minuteswest * 60UL;
#endif
}
