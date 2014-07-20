//#include <stdio.h>
//
//int main(void)
//{
//	FILE *fd;
//
//	printf("Calling fopen(...\n");
//
//	fd = fopen("test.txt", "w+");
//	if (!fd) {
//		printf("fopen() returned NULL\n");
//		return 1;
//	}
//
//	printf("fopen() succeeded\n");
//	fclose(fd);
//
//	return 0;
//}

#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>

#define MAX 2

pthread_t tid[MAX];
int counter;
pthread_mutex_t lock;

void* doSomeThing(void *arg)
{
	printf("\tBefore lock\n");
	fflush(0);
	pthread_mutex_lock(&lock);

	unsigned long i = 0;
	printf("\tInside lock\n");
	fflush(0);
	for(i=0; i<(1000000);i++);

	counter += 1;
	pthread_mutex_unlock(&lock);

	printf("\tAfter lock\n");
	fflush(0);
	
	return NULL;
}

int main(void)
{
	int i = 0;
	int err;

	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n mutex init failed\n");
		return 1;
	}

	while(i < MAX) {
		err = pthread_create(&(tid[i]), NULL, &doSomeThing, NULL);
		if (err != 0)
			printf("\ncan't create thread :[%s]", strerror(err));
		i++;
	}

	i = 0;
	while (i < MAX)
		pthread_join(tid[i++], NULL);

	pthread_mutex_destroy(&lock);

	return 0;
}
