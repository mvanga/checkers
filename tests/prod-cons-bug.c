#include <stdio.h>
#include <pthread.h>
#include <string.h>

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
		//buffer[size] = 1;
		size++;
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}

void * consume (void *arg) {
	int i;
	for(i = 0; i < TRIALS; i++) {
		pthread_mutex_lock(&lock);
		//buffer[size-1]++;
		size--;
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}

void setup() {
	int i, err;
	size = 0;

	if (pthread_mutex_init(&lock, NULL) != 0)
		printf("\n mutex init failed\n");
	
	for (i = 0; i < NUM_PROD; i++) {	
		err = pthread_create(&threads[i], NULL, produce, NULL);
		if (err != 0)
			printf("\ncan't create thread :[%s]", strerror(err));
	}

	for (i = 0; i < NUM_CONS; i++) {
		err = pthread_create(&threads[i+NUM_PROD], NULL, produce, NULL);
		if (err != 0)
			printf("\ncan't create thread :[%s]", strerror(err));
	}
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