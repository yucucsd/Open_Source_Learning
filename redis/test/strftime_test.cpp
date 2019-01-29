#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
int main(){
	struct timeval* tv; 
	tv = (struct timeval*) malloc (sizeof(struct timeval));
	char buf[64];
	gettimeofday(tv, NULL);
	struct tm tm;
	int off = strftime(buf, sizeof(buf), "%d %b %Y %H:%M:%S.", &tm);
	snprintf(buf+off, sizeof(buf) - off, "%03d", (int)tv->tv_sec/1000);
	printf("%s\n", buf);
}
