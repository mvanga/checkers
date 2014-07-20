#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#define MAXTHREADS 1000

//#define debug(...) do { printf(__VA_ARGS__); fflush(0); } while(0);
#define debug(...)

/* Structure for global chess data */
struct chess_data {
	int running;					/* Whether chess should run or not */
	int num_threads;				/* Number of threads */
	pthread_mutex_t global_lock;	/* Lock for global data */
	pthread_t scheduler;			/* Pthread ID of main chess thread */
	int next_thread;				/* The thread ID that should run next */
	int last_thread;				/* The thread ID that executed last */
	int num_active;					/* Number of active threads */
	int active[MAXTHREADS];			/* Bitmap of active threads */
	int sched_start;				/* Whether scheduler should start or not */

	const char *mode;				/* The mode: either 'record' or 'replay' */
	const char *file;				/* The filename to record to/replay from */
	FILE *filefd;

	pthread_cond_t sched_start_cv; 	/* Condition variable for starting */
	pthread_cond_t all_threads_cv; 	/* All threads are ready */
	pthread_cond_t sched_change_cv;	/* Condition variable for the scheduler */
};

/* Per-thread information */
struct thread_data {
	int tid;
};

struct chess_data *chess_data;
struct thread_data __thread thread_data;

/* Pointers to the original pthread functions */
int (*__pthread_create)(pthread_t *, const pthread_attr_t *, void *(*start_routine)(void*), void *);
int (*__pthread_join)(pthread_t, void **);
int (*__pthread_mutex_lock)(pthread_mutex_t *);
int (*__pthread_mutex_unlock)(pthread_mutex_t *);
int (*__pthread_cond_wait)(pthread_cond_t *, pthread_mutex_t *);
int (*__pthread_cond_signal)(pthread_cond_t *, pthread_mutex_t *);
int (*__pthread_cond_broadcast)(pthread_cond_t *, pthread_mutex_t *);

void wake_scheduler()
{
	chess_data->next_thread = -1;
	__pthread_cond_broadcast(&chess_data->sched_change_cv, &chess_data->global_lock);
}

void block_and_wait_for_turn(int call_scheduler_next)
{
	if (call_scheduler_next)
		wake_scheduler();
	else
		__pthread_cond_broadcast(&chess_data->sched_change_cv, &chess_data->global_lock);

	while (chess_data->next_thread != thread_data.tid) {
		debug("=====>UnLocking %p THREAD %d.\n", &chess_data->global_lock, thread_data.tid);
		__pthread_cond_wait(&chess_data->sched_change_cv, &chess_data->global_lock);
		debug("=====>Locking %p THREAD %d.\n", &chess_data->global_lock, thread_data.tid);
	}
}

void block_and_wait_for_turn_locked(int call_scheduler_next)
{
	debug("abc1\n");
	__pthread_mutex_lock(&chess_data->global_lock);
	debug("abc2\n");
	block_and_wait_for_turn(call_scheduler_next);
	__pthread_mutex_unlock(&chess_data->global_lock);
}

void *chess_scheduler(void *arg)
{
	thread_data.tid = -1;
	
	/* Wait for first call of sched_yield() */
	debug("SCHED: Initialized. Waiting...\n");
	__pthread_mutex_lock(&chess_data->global_lock);
	while (!chess_data->sched_start)
		__pthread_cond_wait(&chess_data->sched_start_cv, &chess_data->global_lock);
	
	debug("SCHED STARTED!\n");
	chess_data->last_thread = -1;
	while (chess_data->running) {
		if (strcmp(chess_data->mode, "record") == 0) {
			/* For record, we just use a braindead round-robin scheduler */
			do {
				chess_data->next_thread = (chess_data->last_thread + 1) % chess_data->num_threads;
			} while (!chess_data->active[chess_data->next_thread]);
			/* Write our choice into the file */
			fprintf(chess_data->filefd, "%d\n", chess_data->next_thread);
		} else if (strcmp(chess_data->mode, "replay") == 0) {
			/* For replay, we read the next thread to scheduler from the file */
			fscanf(chess_data->filefd, "%d\n", &chess_data->next_thread);
		}
		chess_data->last_thread = chess_data->next_thread;
		debug("Picked %d\n", chess_data->next_thread);
		block_and_wait_for_turn(0);
	}
	__pthread_mutex_unlock(&chess_data->global_lock);
	
	debug("SCHED: There are no threads anymore. Exiting...\n");
	
	return NULL;
}

/* Structure to pass to each thread that we create */
struct thread_params {
	int tid;
	void *(*start_routine)(void*);
	void *arg;
};

//void *chess_scheduler(void *arg);

/* Define a library constructor so we can initialize global stuff */
static void __attribute__((constructor)) chess_init(void)
{
	/* Allocate and initialize our global data structure */
	chess_data = malloc(sizeof(struct chess_data));
	if (!chess_data) {
		debug("chess: failed to allocate global chess data\n");
		exit(EXIT_FAILURE);
	}

	chess_data->next_thread = -1;
	chess_data->running = 1;
	chess_data->num_threads = 0;
	chess_data->sched_start = 0;
	chess_data->num_active = 0;
    pthread_mutex_init(&chess_data->global_lock, NULL);
    pthread_cond_init(&chess_data->sched_change_cv, NULL);
    pthread_cond_init(&chess_data->sched_start_cv, NULL);
    pthread_cond_init(&chess_data->all_threads_cv, NULL);
	memset(chess_data->active, 0, sizeof(int)*MAXTHREADS);
	chess_data->mode = getenv("MODE");
	chess_data->file = getenv("FILE");

	if (strcmp(chess_data->mode, "record") == 0) {
		chess_data->filefd = fopen(chess_data->file, "w");
		if (chess_data->filefd  == NULL) {
			debug("chess: could not open record file: %s\n", chess_data->file);
			exit(EXIT_FAILURE);
		}
	} else if (strcmp(chess_data->mode, "replay") == 0) {
		chess_data->filefd = fopen(chess_data->file, "r");
		if (chess_data->filefd  == NULL) {
			debug("chess: could not open replay file: %s\n", chess_data->file);
			exit(EXIT_FAILURE);
		}
	} else {
		debug("chess: unknown mode: %s\n", chess_data->mode);
		exit(EXIT_FAILURE);
	}


	printf("Mode: %s\n", chess_data->mode);
	printf("File: %s\n", chess_data->file);

	/* Get pointers to the original pthread library functions */
	__pthread_create = dlsym(RTLD_NEXT, "pthread_create");
	__pthread_join = dlsym(RTLD_NEXT, "pthread_join");
	__pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	__pthread_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	__pthread_cond_wait = dlsym(RTLD_NEXT, "pthread_cond_wait");
	__pthread_cond_signal = dlsym(RTLD_NEXT, "pthread_cond_signal");
	__pthread_cond_broadcast = dlsym(RTLD_NEXT, "pthread_cond_broadcast");

	/* Create our chess scheduler thread that controls the others */
	if (__pthread_create(&chess_data->scheduler, NULL, chess_scheduler, NULL)) {
		debug("chess: failed to create scheduler thread\n");
		exit(EXIT_FAILURE);
	}
}

static void __attribute__((destructor)) chess_exit(void)
{
	if (!chess_data)
		return;

	chess_data->running = 0;
	__pthread_join(chess_data->scheduler, NULL);
	fclose(chess_data->filefd);
	free(chess_data);
}

void thread_start()
{
	/* Start blocked and wait for the scheduler */
	__pthread_mutex_lock(&chess_data->global_lock);
	chess_data->active[thread_data.tid] = 1;
	chess_data->num_active++;
	__pthread_cond_signal(&chess_data->all_threads_cv, &chess_data->global_lock);

	debug("THREAD %d: waiting...\n", thread_data.tid);
	//debug("bla0 %d\n", thread_data.tid);
	block_and_wait_for_turn(1);
	//debug("bla4 %d\n", thread_data.tid);
	__pthread_mutex_unlock(&chess_data->global_lock);
	debug("THREAD %d: starting...\n", thread_data.tid);
}

void thread_exit()
{
	/* Mark thread as inactive, so it is not scheduled anymore */
	debug("whee\n");
	__pthread_mutex_lock(&chess_data->global_lock);
	debug("whee2\n");
	chess_data->active[thread_data.tid] = 0;
	chess_data->num_active--;
	if (chess_data->num_active == 0)
		chess_data->running = 0;
	
	debug("THREAD %d: Exited. Waking up the scheduler!\n", thread_data.tid);
	
	/* We don't want to block, so just wake scheduler */
	wake_scheduler();
	__pthread_mutex_unlock(&chess_data->global_lock);
}

void *thread_run(void *data)
{
	void *arg;
	struct thread_params *params;
	void *(*start_routine)(void *);
	
	/* Extract out the data from our thread_data and free it */
	params = (struct thread_params *)data;
	thread_data.tid = params->tid;
	arg = params->arg;
	start_routine = params->start_routine;
	free(params);

	thread_start();
	start_routine(arg);
	thread_exit();
	
	return NULL;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
	struct thread_params *params;
	
	/* Create structure to pass thread data as parameters */
	params = malloc(sizeof(struct thread_params));
	if (!params)
		return -ENOMEM;

	params->tid = chess_data->num_threads;
	params->start_routine = start_routine;
	params->arg = arg;

	/* We don't need a lock for this since pthread_creates are serialized */
	chess_data->num_threads++;

	return __pthread_create(thread, attr, thread_run, params);
}

int pthread_mutex_lock(pthread_mutex_t *t)
{
	debug("THREAD %d: Waiting for lock. [%p]\n", thread_data.tid, t);
	block_and_wait_for_turn_locked(1);
	debug("THREAD %d: Lock acquired. [%p]\n", thread_data.tid, t);	
	return __pthread_mutex_lock(t);
}

int pthread_mutex_unlock(pthread_mutex_t *t)
{
	int ret;
	debug("THREAD %d: Releasing lock. [%p]\n", thread_data.tid, t);
	ret = __pthread_mutex_unlock(t);
	debug("done\n");
	block_and_wait_for_turn_locked(1);
	debug("THREAD %d: Lock released. [%p]\n", thread_data.tid, t);
	return ret;
}

int pthread_join(pthread_t p, void **ret)
{
	thread_data.tid = 9999;
	__pthread_mutex_lock(&chess_data->global_lock);
	if (!chess_data->sched_start) {
		debug("START: Waiting for %d threads to be ready. num_active: %d\n", chess_data->num_threads, chess_data->num_active);
		
		/* Wait for every thread to be ready */
		while (chess_data->num_active < chess_data->num_threads) {
			debug("=====>UnLocking %p THREAD %d. \n", &chess_data->global_lock, thread_data.tid);
			__pthread_cond_wait(&chess_data->all_threads_cv, &chess_data->global_lock);
			debug("=====>Locking %p THREAD %d.\n", &chess_data->global_lock, thread_data.tid);
		}
		
		chess_data->sched_start = 1;

		debug("START: Threads are ready.\n");
		debug("START: Waking up the scheduler.\n");
		__pthread_cond_signal(&chess_data->sched_start_cv, &chess_data->global_lock);
	}
	__pthread_mutex_unlock(&chess_data->global_lock);

	return __pthread_join(p, ret);
}
