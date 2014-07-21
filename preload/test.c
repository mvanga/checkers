#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>

#define MAX 2

pthread_t tid[MAX];

int counter;
pthread_mutex_t lock;

void* worker(void *arg)
{
	pthread_mutex_lock(&lock);

	unsigned long i = 0;
	for(i=0; i<(1000000);i++);
	counter += 1;

	pthread_mutex_unlock(&lock);

	return arg;
}

int init(void)
{
	int err;
	int i = 0;

	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("Failed to initialize mutex\n");
		return 1;
	}

	while(i < MAX) {
		err = pthread_create(&(tid[i]), NULL, &worker, NULL);
		if (err != 0)
			printf("Failed to create thread:[%s]", strerror(err));
		i++;
	}

	return 0;
}

int run(void)
{
	int i = 0;

	while (i < MAX)
		pthread_join(tid[i++], NULL);

	/* Do some checks here and decide return value based on this */

	return 0;
}

void deinit(void)
{
	pthread_mutex_destroy(&lock);
}

int main(void)
{
	int ret;

	ret = init();
	if (ret)
		exit(ret);
	ret = run();
	deinit();

	return ret;
}
