#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <pthread.h>

//任务链表
typedef struct tpool_work {
	void* (routine)(void*);	//任务函数
	void *arg;			//函数参数
	struct tpool_work *next;
}tpool_work_t;

typedef struct tpool {
	int shutdown;
	int max_thr_num;			//线程数量
	pthread_t *thr_id;			//线程id数组
	tpool_work_t *queue_head;	//线程链表
	pthread_mutex_t queue_lock;
	pthread_cond_t queue_ready;
}tpool_t;

/*
 * @brief	创建线程池
 * @param	max_thr_num 最大线程数
 * @return	0:成功
 */
int tpool_create(int max_thr_num);

/*
 * @brief	向线程池中提交任务
 * @param	任务函数指针
 * @return	0：成功
 */
int tpool_submit_work(void* (*routine)(void*), void* arg);

#endif
