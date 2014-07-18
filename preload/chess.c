#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>
#include <semaphore.h>

int num_threads;
sem_t barrier;
//sem_t lock[1000];

__attribute__((constructor)) void init(void) {
	num_threads = 0;
	if(sem_init(&barrier, 0, 0) != 0) {
		printf("Could not initialize semaphore to 0!\n");
	}
}


void *my_blocker(void *arg)
{
	int value;
	void *(*start_routine)(void *);
	start_routine = arg;

	//printf("My Thread ID: %d\n", pthread_self());
	printf("wheee\n");
	if (sem_getvalue(&barrier, &value) != 0)
		printf("Could not read semaphore!\n");
	
	printf("Value: %d\n", value);
	
	sem_wait(&barrier);
	
	start_routine(NULL);
	return NULL;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{	
	printf("Creating pthread\n");
	fflush(0);

	/*if(sem_init(&lock[num_threads], 0, 0) != 0) {
		printf("Could not initialize semaphore to 0!\n");
	}*/

	num_threads++;

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
	int value;
	printf("Time to start!!!\n");
	fflush(0);

	if (sem_getvalue(&barrier, &value) != 0)
		printf("Could not read semaphore!\n");
	
	printf("There are %d blocked threads. Semaphore: %d\n", num_threads, value);
	/*sem_post(&barrier);
	sem_getvalue(&barrier, &value);
	printf("decreased to Value: %d\n", value);
	if (value < 0) {
		
	}
	sem_wait(&barrier);*/

	int (*original)(pthread_t, void **);
	original = dlsym(RTLD_NEXT, "pthread_join");
	return (*original)(p, ret);
}
