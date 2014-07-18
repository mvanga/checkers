#include <stdio.h>
#include <pthread.h>

#define NUM_PROD 5
#define NUM_CONS 5
#define MAXSIZE 1000
#define TRIALS 100

int buffer[MAXSIZE];
int size;
int error;

pthread_mutex_t lock;
pthread_t threads[NUM_PROD + NUM_CONS];


void * produce (void *arg) {
	int i;
	for(i = 0; i < TRIALS; i++) {
		pthread_mutex_lock(&lock);
		if (size < MAXSIZE) {
			buffer[size] = 1;
			size++;
			//printf("produced. size = %d\n", size);
		}
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}

void * consume (void *arg) {
	int i;
	for(i = 0; i < TRIALS; i++) {
		pthread_mutex_lock(&lock);
		if (size > 0) {
			buffer[size-1]++;
			size--;
			//printf("consumed. size = %d\n", size);
		}
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}

void setup() {
	int i;
	size = 0;

	for (i = 0; i < NUM_PROD; i++)
		pthread_create(&threads[i], NULL, produce, NULL);

	for (i = 0; i < NUM_CONS; i++)
		pthread_create(&threads[i+NUM_PROD], NULL, consume, NULL);
}

int run() {
	int i;
	for (i = 0; i < NUM_PROD+NUM_CONS; i++)
		pthread_join(threads[i], NULL);	
	
	return (size >= 0 && size <= MAXSIZE);
}

int main() {
	setup();
	int ret = run();
	printf("OK: %d\n", ret);
	
	return 0;
}