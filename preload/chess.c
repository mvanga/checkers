#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#define MAXTHREADS 1000

/* Structure for global chess data */
struct chess_data {
	int running;					/* Whether chess should run or not */
	int num_threads;				/* Number of threads */
	pthread_mutex_t sched_mutex;	/* Lock for global data */
	pthread_cond_t sched_cv;		/* Condition variable to wait on */
	pthread_t scheduler;			/* Pthread ID of main chess thread */
	int next_thread;				/* The thread ID that should run next */
};

/* Per-thread information */
struct thread_data {
	int tid;
};

struct chess_data *chess_data;
struct thread_data __thread thread_data;

int sched_started = 0;

__thread int tid;
int ready_counter = 0;

pthread_mutex_t sched_mutex;
pthread_cond_t sched_cv;
pthread_cond_t thread_cv;
int active[MAXTHREADS];

void *pick_thread(void *arg) {
	/* Wait for first call of sched_yield() */
	__pthread_mutex_lock(&sched_mutex);
	printf("SCHED: Initialized. Waiting...\n");
	__pthread_cond_wait(&sched_cv, &sched_mutex);
	__pthread_mutex_unlock(&sched_mutex);
	
	printf("SCHED STARTED!\n");
	
	while (1) {
		__pthread_mutex_lock(&sched_mutex);
		
		if (ready_counter == 0) {
			/* Finish execution if there are no threads */
			printf("SCHED: There are no threads anymore. Exiting...\n");
			__pthread_mutex_unlock(&sched_mutex);
			return NULL;
		}
		
		do {
			current_thread = (current_thread + 1) % num_threads;
		} while (!active[current_thread]);

		printf("SCHED: current_thread is now: %d.\n", current_thread);
		
		printf("SCHED: waking up thread %d. Waiting...\n", current_thread);
		__pthread_cond_broadcast(&thread_cv, &sched_mutex);
		__pthread_cond_wait(&sched_cv, &sched_mutex);
	printf("THREAD %d: waking up the scheduler. Waiting...\n", tid);
	__pthread_cond_signal(&sched_cv, &sched_mutex);
	do {
		__pthread_cond_wait(&thread_cv, &sched_mutex);
	} while (current_thread != tid);
	
	__pthread_mutex_unlock(&sched_mutex);
}

/* Pointers to the original pthread functions */
int (*__pthread_create)(pthread_t *, const pthread_attr_t *, void *(*start_routine)(void*), void *);
int (*__pthread_mutex_lock)(pthread_mutex_t *);
int (*__pthread_mutex_unlock)(pthread_mutex_t *);
int (*__pthread_cond_wait)(pthread_cond_t *, pthread_mutex_t *);
int (*__pthread_cond_signal)(pthread_cond_t *, pthread_mutex_t *);
int (*__pthread_cond_broadcast)(pthread_cond_t *, pthread_mutex_t *);

/* Structure to pass to each thread that we create */
struct thread_params {
	int tid;
	void *(*start_routine)(void*);
	void *arg;
};

void *chess_scheduler(void *arg);

/* Define a library constructor so we can initialize global stuff */
static void __attribute__((constructor)) chess_init(void)
{
	/* Allocate and initialize our global data structure */
	chess_data = malloc(sizeof(struct chess_data));
	if (!chess_data) {
		printf("chess: failed to allocate global chess data\n");
		exit(1);
	}

	chess_data->current_thread = -1;
	chess_data->running = 1;
	chess_data->num_threads = 0;
    pthread_mutex_init(&chess_data->sched_mutex, NULL);
    pthread_cond_init(&chess_data->sched_cv, NULL);
	memset(&active, 0, sizeof(int)*MAXTHREADS);

	/* Get pointers to the original pthread library functions */
	__pthread_create = dlsym(RTLD_NEXT, "pthread_create");	
	__pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	__pthread_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	__pthread_cond_wait = dlsym(RTLD_NEXT, "pthread_cond_wait");
	__pthread_cond_signal = dlsym(RTLD_NEXT, "pthread_cond_signal");

	/* Create our chess scheduler thread that controls the others */
	if (__pthread_create(&chess_data->scheduler, NULL, chess_scheduler, NULL)) {
		printf("chess: failed to create scheduler thread\n");
		exit(1);
	}
}

static void __attribute__((destructor)) chess_exit(void)
{
	int ret;

	if (!chess_data)
		return;

	chess_data->running = 0;
	__pthread_join(&chess_data->scheduler, &ret);
	free(chess_data);
}

void *chess_scheduler(void *arg)
{
	tid = -1;

	/* Wait till the scheduler is activated */

	/* Loop forever while we should run */
	while (chess_data->running) {
		block_and_wait_for_turn();
	}

	return NULL;
}

void *thread_run(void *data)
{
	void *arg;
	struct thread_params *params;
	void *(*start_routine)(void *);
	
	/* Extract out the data from our thread_data and free it */
	params = (struct thread_params *)data;
	tid = params->tid;
	arg = params->arg;
	start_routine = params->start_routine;
	free(params);

    pthread_cond_init(&thread_cv, NULL);
	
	thread_start();
	start_routine(arg);
	thread_exit();
	
	return NULL;
}

void block_and_wait_for_turn(void)
{
	__pthread_mutex_lock(&chess_data->sched_lock);
	while (chess_data->next_thread != tid)
		pthread_cond_wait(&chess_data->sched_cv, &chess_data->sched_lock);
	/* If we reach here, it's our turn to run. unlock and return */
	__pthread_mutex_unlock(&chess_data->sched_lock);
}

void thread_start()
{
	/* Start blocked and wait for the scheduler */
	__pthread_mutex_lock(&sched_mutex);
	active[tid] = 1;
	ready_counter++;
	
	printf("THREAD %d: waiting...\n", tid);
	while (current_thread != tid)
		__pthread_cond_wait(&thread_cv, &sched_mutex);
	__pthread_mutex_unlock(&sched_mutex);
	printf("THREAD %d: starting...\n", tid);
}

void thread_exit()
{
	/* Mark thread as inactive, so it is not scheduled anymore */
	__pthread_mutex_lock(&sched_mutex);
	active[tid] = 0;
	ready_counter--;
	printf("THREAD %d: Exited. Waking up the scheduler!\n", tid);
	__pthread_cond_signal(&sched_cv, &sched_mutex);
	__pthread_mutex_unlock(&sched_mutex);
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
	struct thread_params *params;
	
	/* Create structure to pass thread data as parameters */
	params = malloc(sizeof(struct thread_params));
	if (!params)
		return -ENOMEM;

	params->tid = num_threads;
	params->start_routine = start_routine;
	params->arg = arg;

	/* We don't need a lock for this since pthread_creates are serialized */
	num_threads++;

	return __pthread_create(thread, attr, thread_run, params);
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
			__pthread_mutex_lock(&sched_mutex);
			if (ready_counter < num_threads) {
				__pthread_mutex_unlock(&sched_mutex);
				sched_yield();
			} else {
				__pthread_mutex_unlock(&sched_mutex);
				break;
			}
		}
		printf("START: Threads are ready.\n");

		printf("START: Waking up the scheduler.\n");
		__pthread_cond_signal(&sched_cv, &sched_mutex);
	}

	int (*original)(pthread_t, void **);
	original = dlsym(RTLD_NEXT, "pthread_join");
	return (*original)(p, ret);
}
