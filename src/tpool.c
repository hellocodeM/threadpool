#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "tpool.h"

tpool_t *tpool;

static void* t_routine(void *arg) {
	tpool_work_t *work;

	while (1) {
		pthread_mutex_lock(&tpool->queue_lock);
		while (!tpool->works->next && !tpool->shutdown) {
			pthread_cond_wait(&tpool->queue_cond, &tpool->queue_lock);
		}
		//关闭线程池
		if (tpool->shutdown) {
			pthread_mutex_unlock(&tpool->queue_lock);
			pthread_exit(NULL);
		}
		work = tpool->works->next;
		tpool->works->next = tpool->works->next->next;
		pthread_mutex_unlock(&tpool->queue_lock);
		//开始执行函数
		work->routine(work->arg);
		free(work);
	}
	return NULL;
}

int tpool_create(int pool_size) {		
	//初始化线程池
	tpool = malloc(sizeof(tpool_t));
	if (!tpool) {
		perror("tpool_create failed at malloc");
		return -1;
	}
	tpool->shutdown = 0;
	tpool->pool_size = pool_size;
	//初始化线程ID
	tpool->threads = calloc(pool_size, sizeof(pthread_t));
	if (!tpool->threads) {
		perror("toop_create failed at calloc");
		return -1;
	}
	//初始化mutex和cond
	if (pthread_cond_init(&tpool->queue_cond, NULL) != 0) {
		perror("tpool_create failed at pthread_cond_init");
		return -1;
	}
	if (pthread_mutex_init(&tpool->queue_lock, NULL) != 0) {
		perror("tpool_craete failed at pthread_mutex_init");
		return -1;
	}
	//初始化worker线程
	tpool->works = malloc(sizeof(tpool_work_t));
	tpool->works->next = NULL;
	for (int i = 0; i < pool_size; i++) {
		if (pthread_create(&tpool->threads[i], NULL, (void*)t_routine, NULL) != 0) {
			perror("pthread_create failed");
		}
	}
	return 0;
}

int tpool_destroy() {
	tpool->shutdown = 1;
	perror("waiting for every thread end");
	//唤醒所有工作线程	
	pthread_mutex_lock(&tpool->queue_lock);
	pthread_cond_broadcast(&tpool->queue_cond);
	pthread_mutex_unlock(&tpool->queue_lock);
	//等待每个工作线程结束
	for (int i = 0; i < tpool->pool_size; i++) {
		pthread_join(tpool->threads[i], NULL);
	}
	free(tpool->threads);
	pthread_cond_destroy(&tpool->queue_cond);
	pthread_mutex_destroy(&tpool->queue_lock);
	free(tpool);
	return 0;
}

int tpool_submit_work(void* (*routine)(void*), void* arg) {
	//初始化一个工作
	tpool_work_t *work;
	work = malloc(sizeof(tpool_work_t));
	if (!work) {
		perror("submit work failed");
		return -1;
	}

	work->routine = routine;
	work->arg = arg;
	work->next = NULL;
	
	//向队列中添加工作
	pthread_mutex_lock(&tpool->queue_lock);
	
	tpool_work_t *tmp = tpool->works;
	while (tmp->next) {
		tmp = tmp->next;
	}
	tmp->next = work;
	pthread_cond_signal(&tpool->queue_cond);
	pthread_mutex_unlock(&tpool->queue_lock);
	return 0;
}
