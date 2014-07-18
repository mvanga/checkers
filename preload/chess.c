#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>

void *my_blocker(void *arg)
{
	void *(*start_routine)(void *);
	start_routine = arg;
	printf("whee\n");
	start_routine(NULL);
	return NULL;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
	printf("Creating pthread\n");
	fflush(0);

	int (*original)(pthread_t *, const pthread_attr_t *, void *(*start_routine)(void*), void *);
	original = dlsym(RTLD_NEXT, "pthread_create");
	return (*original)(thread, attr, my_blocker, start_routine);
}

int pthread_mutex_lock(pthread_mutex_t *t)
{
	printf("Locking in pthread\n");
	fflush(0);

	int (*original)(pthread_mutex_t *);
	original = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	return (*original)(t);
}

int pthread_mutex_unlock(pthread_mutex_t *t)
{
	printf("Unlocking in pthread\n");
	fflush(0);

	int (*original)(pthread_mutex_t *);
	original = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	return (*original)(t);
}

int pthread_join(pthread_t p, void **ret)
{
	printf("Joining pthread\n");
	fflush(0);

	int (*original)(pthread_t, void **);
	original = dlsym(RTLD_NEXT, "pthread_join");
	return (*original)(p, ret);
}
