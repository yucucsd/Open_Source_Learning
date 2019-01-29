#include <cstdlib>
#include <sys/ioctl.h>
#include <cstdio>
#include <errno.h>
#include <cstring>
#include <string>
#include <iostream>

static struct winsize ws;
size_t progress_printed; /* Printed chars in screen-wid progress bar. */
size_t progress_full; /* How many chars to write to fill the progress bar. */

void memtest_progress_start(std::string title, int pass)
{
	int j;
	printf("\x1b[H\x1b[2J"); /* Cursor home, clear screen. */
	/* Fill with dots. */
	/*for (j = 0; j < ws.ws_col * (ws.ws_row - 2); ++j)
		printf(".");*/
	printf("Please keep the test running several minutes per GB of memory.\n");
	//printf("\x1b[H\x1b[2K"); /* Cursor Home, clear current line. */
	std::cout << title;
	printf(" [%d] round pass.\n", pass);
	progress_printed = 0;
	progress_full = ws.ws_col * (ws.ws_row - 3);
	fflush(stdout);
}

void memtest_progress_end(void)
{
	printf("\x1b[H\x1b[2J"); /* Cursor home, clear screen. */
}


/* Print the testing progress. */
void memtest_progress_step(unsigned long curr, size_t size, char c)
{
	size_t chars = ((unsigned long long)curr * progress_full) / size, j;
	for (j = 0; j < chars - progress_printed; j++)
		printf("%c", c);
	progress_printed = chars;
	fflush(stdout);
}

/* Test that addressing is fine. Every location is populated with its own 
address, and finally verified. This test is very fast but may detect ASAP
big issues with many memory subsystem. */
int memtest_addressing(unsigned long *l, size_t bytes, int interactive)
{
	unsigned long words = bytes/sizeof(unsigned long);
	unsigned long j, *p;
	
	/* Fill */
	p = l;
	for (j = 0; j < words; ++j)
	{
		*p = (unsigned long)p;
		++p;
		/*if((j & 0xffff) == 0 && interactive)
			memtest_progress_step(j, words * 2, 'A');*/ 
	}
	p = l;
	/* Test */
	for (j = 0; j < words; ++j)
	{
		if (*p != (unsigned long)p)
		{
			if (interactive)
			{
				printf("\n*** MEMORY ADDREESSING EERROR: %p contains %lu\n",
					(void*) p, *p);
				exit(1);
			}
			return 1;
		}
		++p;
		/*if ((j & 0xffff) == 0 && interactive)
			memtest_progress_step(j + words, words * 2, 'A');*/
	}
	return 0;
}

int memtest_test(unsigned long *m, size_t bytes, int passes, int interactive)
{
	int pass = 0;
	int errors = 0;
	
	while (pass != passes)
	{
		++pass;
		if (interactive)
			memtest_progress_start("Addressing test", pass);
		errors += memtest_addressing(m, bytes, interactive);
		if (interactive)
			memtest_progress_end();
		 
	}
}

/* Perform an interactive test by allocating the specified number of megabytes. */
void memtest_alloc_and_test(size_t megabytes, int passes)
{
	size_t bytes = megabytes * 1024 * 1024;
	unsigned long *m;
	m  = (unsigned long*)malloc(bytes);
	if (!m)
	{
		/* %zu : format for unsigned_int */
		fprintf(stderr, "Unable to allocate %zuMB: %s\n",
			megabytes, strerror(errno));
		exit(1);
	}
	memtest_test(m, bytes, passes, 1);
	free(m);
}

void memtest(size_t megabytes, int passes)
{
	/* make sure that stdout window is open */
	if (ioctl(1, TIOCGWINSZ, &ws) < 0)
	{
		ws.ws_col = 80;
		ws.ws_row = 10;
	}
	printf("ws_col = %zu, ws_row = %zu\n", ws.ws_col, ws.ws_row);
	memtest_alloc_and_test(megabytes, passes);
	printf("Memory size %dMB has passed the test.\n", megabytes);
	exit(0);
}
