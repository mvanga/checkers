#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>

#define MAXTHREADS 1000

pthread_t scheduler;
int sched_started = 0;

int num_threads;
__thread int tid;

int current_thread;
pthread_mutex_t sched_mutex;
pthread_cond_t sched_cv;
pthread_cond_t thread_cv[MAXTHREADS];

/* Original functions that we reuse */
int (*orig_pthread_create)(pthread_t *, const pthread_attr_t *, void *(*start_routine)(void*), void *);
int (*orig_pthread_mutex_lock)(pthread_mutex_t *);
int (*orig_pthread_mutex_unlock)(pthread_mutex_t *);
int (*orig_pthread_cond_wait)(pthread_cond_t *, pthread_mutex_t *);
int (*orig_pthread_cond_signal)(pthread_cond_t *, pthread_mutex_t *);

struct thread_params { int tid; void *(*start_routine)(void*); void *arg; };

void *pick_thread(void *arg) {
	/* Wait for first call of sched_yield() */
	orig_pthread_mutex_lock(&sched_mutex);
	printf("SCHED: pthread_cond_wait(&tsched_cv, &sched_mutex) [Waiting for thread creation].\n");
	orig_pthread_cond_wait(&sched_cv, &sched_mutex);
	orig_pthread_mutex_unlock(&sched_mutex);
	
	printf("SCHED STARTED!\n");
	
	while (1) {
		orig_pthread_mutex_lock(&sched_mutex);
		current_thread = (current_thread + 1) % num_threads;
		
		printf("SCHED: current_thread is now: %d.\n", current_thread);
		
		printf("SCHED: pthread_cond_signal(&thread_cv[%d], &sched_mutex).\n", current_thread);
		orig_pthread_cond_signal(&thread_cv[current_thread], &sched_mutex);
		printf("SCHED: pthread_cond_wait(&sched_cv, &sched_mutex).\n");
		fflush(0);
		orig_pthread_cond_wait(&sched_cv, &sched_mutex);

		orig_pthread_mutex_unlock(&sched_mutex);
	}
	
	return NULL;
}

void thread_yield() {
	orig_pthread_mutex_lock(&sched_mutex);

	printf("THREAD %d: pthread_cond_signal(&sched_cv, &sched_mutex).\n", tid);
	orig_pthread_cond_signal(&sched_cv, &sched_mutex);
	printf("THREAD %d: pthread_cond_wait(&thread_cv[%d], &sched_mutex).\n", tid, tid);
	orig_pthread_cond_wait(&thread_cv[tid], &sched_mutex);

	orig_pthread_mutex_unlock(&sched_mutex);
}

void *thread_init(void *data)
{
	struct thread_params *params;
	void *(*start_routine)(void *);
	void *arg;
	
	params = (struct thread_params *) data;
	tid = params->tid;
	start_routine = params->start_routine;
	arg = params->arg;
	free(params);

	printf("My Thread ID: %d\n", tid);

	printf("THREAD %d: pthread_cond_init(&thread_cv[%d]).\n", tid, tid);
    pthread_cond_init(&thread_cv[tid], NULL);
	
	thread_yield(); /* Wait for the scheduler */
	
	start_routine(arg);
	return NULL;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
	/* Pass thread data as parameters */
	struct thread_params *params = malloc(sizeof(struct thread_params));
	params->tid = num_threads;
	params->start_routine = start_routine;
	num_threads++;

	return orig_pthread_create(thread, attr, thread_init, params);
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
	printf("Time to start!!!\n");
	fflush(0);

	if (!sched_started) {
		sched_started = 1;
		printf("START: pthread_cond_signal(&sched_cv, &sched_mutex).\n");
		orig_pthread_cond_signal(&sched_cv, &sched_mutex);
	}

	int (*original)(pthread_t, void **);
	original = dlsym(RTLD_NEXT, "pthread_join");
	return (*original)(p, ret);
}

__attribute__((constructor)) void init(void) {
	/* Initialize the library */
	num_threads = 0;
	current_thread = -1;

    pthread_mutex_init(&sched_mutex, NULL);
	printf("pthread_cond_init(&sched_cv).\n");
    pthread_cond_init(&sched_cv, NULL);

	orig_pthread_create = dlsym(RTLD_NEXT, "pthread_create");	
	orig_pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	orig_pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	orig_pthread_cond_wait = dlsym(RTLD_NEXT, "pthread_cond_wait");
	orig_pthread_cond_signal = dlsym(RTLD_NEXT, "pthread_cond_signal");

	if (orig_pthread_create(&scheduler, NULL, pick_thread, NULL)) {
		printf("Can't create scheduler thread.\n");
	}	
}

