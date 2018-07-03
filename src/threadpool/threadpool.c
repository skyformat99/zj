#include "utils.h"
#include "threadpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>

static void * meta_fn(void *_ __unuse);
static void task_new(void * (*) (void *), void *);

struct ThreadPool threadpool = { .addjob = task_new };

struct ThreadTask {
    pthread_cond_t cond_var;
    char __pad[128];
    pthread_mutex_t cond_lock;

    void * (* fn) (void *);
    void *p_param;
};

/* 线程池栈结构 */
static _i pool_siz; // 常备线程数量
static struct ThreadTask **pool_stack;
static _i stack_header;
static pthread_mutex_t stack_header_lock;
static pthread_t tid;

__init static void
pool_init(void){
    utils.ncpu(&pool_siz);

    pthread_mutexattr_t zMutexAttr;

    /*
     * 创建线程池时，先销毁旧锁，再重新初始化之，
     * 防止 fork 之后，在子进程中重建线程池时，形成死锁
     * 锁属性设置为 PTHREAD_MUTEX_NORMAL：不允许同一线程重复取锁
     */
    pthread_mutex_destroy(&stack_header_lock);

    pthread_mutexattr_init(&zMutexAttr);
    pthread_mutexattr_settype(&zMutexAttr, PTHREAD_MUTEX_NORMAL);

    pthread_mutex_init(&stack_header_lock, &zMutexAttr);

    pthread_mutexattr_destroy(&zMutexAttr);

    /*
     * 必须动态初始化为 -1
     * 否则子进程继承父进程的栈索引，将带来异常
     */
    stack_header = -1;

    /* 线程池栈结构空间 */
    if(nil == (pool_stack = __alloc(sizeof(void *) * pool_siz))){
        __fatal(strerror(errno));
    }

    _i i = 0;
    while(i < pool_siz){
        if (0 != pthread_create(&tid, nil, meta_fn, nil)) {
            __fatal(strerror(errno));
        }
        ++i;
    }
}


/*
 * 线程池容量不足时，自动扩容
 * 空闲线程过多时，会自动缩容
 */
static void
task_new(void * (* fn) (void *), void *fn_param){
    pthread_mutex_lock(&stack_header_lock);

    while(0 > stack_header){
        pthread_mutex_unlock(&stack_header_lock);
        pthread_create(&tid, nil, meta_fn, nil);
        pthread_mutex_lock(&stack_header_lock);
    }

    pthread_mutex_lock(&pool_stack[stack_header]->cond_lock);

    pool_stack[stack_header]->fn = fn;
    pool_stack[stack_header]->p_param = fn_param;

    pthread_mutex_unlock(&pool_stack[stack_header]->cond_lock);
    pthread_cond_signal(&(pool_stack[stack_header]->cond_var));

    stack_header--;
    pthread_mutex_unlock(&stack_header_lock);
}

static void *
meta_fn(void *_ __unuse){
    pthread_detach(pthread_self());

    /*
     * 线程池中的线程程默认不接受 cancel
     * 若有需求，在传入的工作函数中更改属性即可
     */
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);

    /* 线程任务桩 */
    struct ThreadTask *self_task = __alloc(sizeof(struct ThreadTask));
    if(nil == self_task){
        __fatal(strerror(errno));
    }

    self_task->fn = nil;

    if(0 != pthread_cond_init(&(self_task->cond_var), nil)){
        __fatal(strerror(errno));
    }

    if(0 != pthread_mutex_init(&(self_task->cond_lock), nil)){
        __fatal(strerror(errno));
    }

loop: pthread_mutex_lock(&stack_header_lock);

    if(stack_header < (pool_siz - 1)){
        pool_stack[++stack_header] = self_task;
        pthread_mutex_unlock(&stack_header_lock);

        pthread_mutex_lock(&self_task->cond_lock);
        while(nil == self_task->fn){
            /* 等待任务到达 */
            pthread_cond_wait( &(self_task->cond_var), &self_task->cond_lock);
        }
        pthread_mutex_unlock(&self_task->cond_lock);

        self_task->fn(self_task->p_param);

        self_task->fn = nil;
        goto loop;
    } else {
        pthread_mutex_unlock(&stack_header_lock);
    }

    /* 清理资源占用 */
    pthread_cond_destroy(&(self_task->cond_var));
    free(self_task);

    pthread_exit(nil);
}
