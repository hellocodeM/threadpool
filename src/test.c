#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "threadpool.h"

void *func(void *arg) {
	printf("thread %d\n", (int)arg);
	return NULL;
}

int main(int argc, char **argv) {
	if (tpool_create(5) != 0) {
		printf("tpool_create failed");
		exit(1);
	}

	for (int i = 0; i < 10; i++) {
		tpool_submit_work(func, (void*)i);
	}
	sleep(2);
	tpool_destroy();
	return 0;
}
