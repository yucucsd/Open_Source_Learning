#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#define serverPanic(...) _serverPanic(__FILE__, __LINE__, __VA_ARGS__),_exit(1)

void _serverPanic(const char* file, int line, const char* msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	char fmtmsg[256];
	printf("msg = %s\n", msg);
	/* if msg does not contain format, msg will be copied */
	vsnprintf(fmtmsg, sizeof(fmtmsg), msg, ap);
	va_end(ap);
	/* Use printf with \n to flush stdout, or use fflush(stdout) */
	printf("Meditation: %s #%s:%d\n", fmtmsg, file, line);

}

int main()
{
	printf("printf OK!\n");
	serverPanic("%s", "Succeed");
	printf("printf end!\n");
}
