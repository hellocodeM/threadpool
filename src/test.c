#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "tpool.h"

void *func(void *arg) {
	printf("thread %d will sleep 1 sec.\n", *(int*)arg);
	sleep(1);
	return NULL;
}

int main(int argc, char **argv) {
	if (tpool_create(5) != 0) {
		printf("tpool_create failed");
		exit(1);
	}

	int num[10];
	for (int i = 0; i < 10; i++) {
		num[i] = i;
	}
	for (int i = 0; i < 10; i++) {
		tpool_submit_work(func, (void*)&num[i]);
	}
	tpool_destroy();
	return 0;
}
