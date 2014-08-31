#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "threadpool.h"

static tpool_t *tpool = NULL;

//工作者线程，从任务链表中取出任务并且执行
static void* thread_routine(void *arg) {
	tpool_work_t *work;

	while (1) {
		pthread_mutex_lock(&tpool->queue_lock);
		while (!tpool->queue_head && !tpool->shutdown) {
			pthread_cond_wait(&tpool->queue_ready, &tpool->queue_lock);
		}
		if (tpool->shutdown) {
			pthread_mutex_unlock(&tpool->queue_lock);
			pthread_exit(NULL);
		}
		work = tpool->queue_head;
		tpool->queue_head = tpool->queue_head->next;
		pthread_mutex_unlock(&tpool->queue_lock);

		work->routine(work->arg);
		free(work);
	}
	return NULL;
}

//创建线程池
int tpool_create(int max_thr_num) {
	tpool = calloc(1, sizeof(tpool_t));
	if (!tpool) {
		printf("calloc failed\n");
		exit(1);
	}
	//初始化
	tpool->max_thr_num = max_thr_num;
	tpool->shutdown = 0;
	tpool->queue_head = NULL;
	if (pthread_mutex_init(&tpool->queue_lock, NULL) != 0) {
		printf("pthread_mutex_init failed");
		exit(1);
	}
	if (pthread_cond_init(&tpool->queue_ready, NULL) != 0) {
		printf("pthread_cond_init failed");
		exit(1);
	}

	//创建工作者线程
	tpool->thr_id = calloc(max_thr_num, sizeof(pthread_t));
	if (!tpool->thr_id) {
		printf("calloc failed");
		exit(1);
	}
	for (int i = 0; i < max_thr_num; ++i) {
		if (pthread_create(&tpool->thr_id[i], NULL, thread_routine, NULL) != 0) {
			printf("pthread_create failed");
			exit(1);
		}
	}
	return 0;
}

//销毁线程池
void tpool_destroy() {

	if (tpool->shutdown) {
		return ;
	}
	tpool->shutdown = 1;
	pthread_mutex_lock(&tpool->queue_lock);
	pthread_cond_broadcast(&tpool->queue_ready);
	pthread_mutex_unlock(&tpool->queue_lock);
	for (int i = 0; i < tpool->max_thr_num; ++i) {
		pthread_join(tpool->thr_id[i], NULL);
	}
	free(tpool->thr_id);

	tpool_work_t *member;
	while (tpool->queue_head) {
		member = tpool->queue_head;
		tpool->queue_head = tpool->queue_head->next;
		free(member);
	}
	pthread_mutex_destroy(&tpool->queue_lock);
	pthread_cond_destroy(&tpool->queue_ready);
	
	free(tpool);
}

int tpool_submit_work(void* (*routine)(void *), void *arg) {
	tpool_work_t *work, *member;

	if (!routine) {
		printf("invalid argument");
		return -1;
	}
	work = malloc(sizeof(tpool_work_t));
	if (!work) {
		printf("malloc failed");
		return -1;
	}
	work->routine = routine;
	work->arg = arg;
	work->next = NULL;

	pthread_mutex_lock(&tpool->queue_lock);
	member = tpool->queue_head;
	if (!member) {
		tpool->queue_head = work;
	} else {
		while (member->next) {
			member = member->next;
		}
		member->next = work;
	}
	pthread_cond_signal(&tpool->queue_ready);
	pthread_mutex_unlock(&tpool->queue_lock);

	return 0;
}
