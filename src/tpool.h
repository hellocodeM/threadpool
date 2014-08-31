#include <pthread.h>

typedef struct tpool_work_s {
	void* (*routine)(void*);
	void *arg;
	struct tpool_work_s *next;	
}tpool_work_t;

typedef struct tpool_s {
	int pool_size;
	int shutdown;
	pthread_t *threads;
	tpool_work_t *works;
	pthread_cond_t queue_cond;
	pthread_mutex_t queue_lock;
}tpool_t;

int tpool_create(int pool_size);

extern int tpool_destroy();

int tpool_submit_work(void* (*routine)(void*), void *arg);

