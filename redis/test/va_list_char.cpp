#include <stdio.h>
#include <stdarg.h>

#define log(...) _log(__VA_ARGS__)

void _log(char* arg, int k)
{
	printf("%s = %d\n", arg, k);
}

void _log(char* arg)
{
	printf("log %s\n", arg);
}

int main()
{
	log ("blabla!");
}
