/* This file implements atomic counters using __atomic or __sync
macros if available, otherwise synchronizing different threads
using a mutex.

If atomic primitives are available (tested in config.h) the mutex
is not used. 

*/
#include <pthread.h>

#ifndef _ATOMIC_VAR_H
#define _ATOMIC_VAR_H

#define atomicSet(var, value) do {\
	pthread_mutex_lock(&var ## _mutex); \
	var = value; \
	pthread_mutex_unlock(&var ## _mutex); \
}while(0)
