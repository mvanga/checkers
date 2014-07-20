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
int ready_counter = 0;

int current_thread;
pthread_mutex_t sched_mutex;
pthread_cond_t sched_cv;
pthread_cond_t thread_cv;
int active[MAXTHREADS];

/* Original functions that we reuse */
int (*orig_pthread_create)(pthread_t *, const pthread_attr_t *, void *(*start_routine)(void*), void *);
int (*orig_pthread_mutex_lock)(pthread_mutex_t *);
int (*orig_pthread_mutex_unlock)(pthread_mutex_t *);
int (*orig_pthread_cond_wait)(pthread_cond_t *, pthread_mutex_t *);
int (*orig_pthread_cond_signal)(pthread_cond_t *, pthread_mutex_t *);
int (*orig_pthread_cond_broadcast)(pthread_cond_t *, pthread_mutex_t *);

struct thread_params { int tid; void *(*start_routine)(void*); void *arg; };

void *pick_thread(void *arg) {
	/* Wait for first call of sched_yield() */
	orig_pthread_mutex_lock(&sched_mutex);
	printf("SCHED: Initialized. Waiting...\n");
	orig_pthread_cond_wait(&sched_cv, &sched_mutex);
	orig_pthread_mutex_unlock(&sched_mutex);
	
	printf("SCHED STARTED!\n");
	
	while (1) {
		orig_pthread_mutex_lock(&sched_mutex);
		
		if (ready_counter == 0) {
			/* Finish execution if there are no threads */
			printf("SCHED: There are no threads anymore. Exiting...\n");
			orig_pthread_mutex_unlock(&sched_mutex);
			return NULL;
		}
		
		do {
			current_thread = (current_thread + 1) % num_threads;
		} while (!active[current_thread]);

		printf("SCHED: current_thread is now: %d.\n", current_thread);
		
		printf("SCHED: waking up thread %d. Waiting...\n", current_thread);
		orig_pthread_cond_broadcast(&thread_cv, &sched_mutex);
		orig_pthread_cond_wait(&sched_cv, &sched_mutex);

		orig_pthread_mutex_unlock(&sched_mutex);
	}
	
	return NULL;
}

void thread_yield() {
	orig_pthread_mutex_lock(&sched_mutex);

	printf("THREAD %d: waking up the scheduler. Waiting...\n", tid);
	orig_pthread_cond_signal(&sched_cv, &sched_mutex);
	do {
		orig_pthread_cond_wait(&thread_cv, &sched_mutex);
	} while (current_thread != tid);
	
	orig_pthread_mutex_unlock(&sched_mutex);
}

void thread_start() {
	/* Start blocked and wait for the scheduler */
	orig_pthread_mutex_lock(&sched_mutex);
	active[tid] = 1;
	ready_counter++;
	
	printf("THREAD %d: waiting...\n", tid);
	do {
		orig_pthread_cond_wait(&thread_cv, &sched_mutex);
	} while (current_thread != tid);
	orig_pthread_mutex_unlock(&sched_mutex);
}

void thread_exit() {
	/* Mark thread as inactive, so it is not scheduled anymore */
	orig_pthread_mutex_lock(&sched_mutex);
	active[tid] = 0;
	ready_counter--;
	printf("THREAD %d: Exited. Waking up the scheduler!\n", tid);
	orig_pthread_cond_signal(&sched_cv, &sched_mutex);
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

    pthread_cond_init(&thread_cv, NULL);
	
	thread_start();
	start_routine(arg);
	thread_exit();
	
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
	printf("THREAD %d: Waiting for lock. [%p]\n", tid, t);
	thread_yield();
	printf("THREAD %d: Lock acquired. [%p]\n", tid, t);	
	return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *t)
{
	printf("THREAD %d: Releasing lock. [%p]\n", tid, t);
	thread_yield();
	printf("THREAD %d: Lock released. [%p]\n", tid, t);

	return 0;
}

int pthread_join(pthread_t p, void **ret)
{
	if (!sched_started) {
		sched_started = 1;
		
		printf("START: Waiting for %d threads to be ready.\n", num_threads);
		
		/* Wait for every thread to be ready */
		while (1) {
			orig_pthread_mutex_lock(&sched_mutex);
			if (ready_counter < num_threads) {
				orig_pthread_mutex_unlock(&sched_mutex);
				sched_yield();
			} else {
				orig_pthread_mutex_unlock(&sched_mutex);
				break;
			}
		}
		printf("START: Threads are ready.\n");

		printf("START: Waking up the scheduler.\n");
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
    pthread_cond_init(&sched_cv, NULL);
	memset(active, 0, sizeof(int)*MAXTHREADS);

	orig_pthread_create = dlsym(RTLD_NEXT, "pthread_create");	
	orig_pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	orig_pthread_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	orig_pthread_cond_wait = dlsym(RTLD_NEXT, "pthread_cond_wait");
	orig_pthread_cond_signal = dlsym(RTLD_NEXT, "pthread_cond_signal");
	orig_pthread_cond_broadcast = dlsym(RTLD_NEXT, "pthread_cond_broadcast");

	if (orig_pthread_create(&scheduler, NULL, pick_thread, NULL)) {
		printf("Can't create scheduler thread.\n");
	}	
}

