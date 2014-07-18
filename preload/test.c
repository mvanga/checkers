#include <stdio.h>

int main(void)
{
	FILE *fd;

	printf("Calling fopen(...\n");

	fd = fopen("test.txt", "w+");
	if (!fd) {
		printf("fopen() returned NULL\n");
		return 1;
	}

	printf("fopen() succeeded\n");
	fclose(fd);

	return 0;
}
