#include <stdio.h>
#include <stdarg.h>
void logva(va_list args)
{
	char* t = va_arg(args, char*);
	printf("%s\n", t);
	va_end(args);
}
void log(char* arg1,...)
{
/* First Form
	va_list args;
	char buf[64];
	va_start(args, arg1);
	char* t = va_arg(args, char*);
	printf("%s\n", t);
	t = va_arg(args, char*);
	printf("%s\n", t);
	va_end(args);
*/
	va_list args;
	va_start(args, arg1);
	logva(args);

}

int main()
{
	logvb("This", "Is", "A", "Test");
}
