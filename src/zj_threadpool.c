#include "zj_threadpool.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static Error *zj_pool_init(_i);
static void zj_task_new(void * (*) (void *), void *);

/* 线程池栈结构 */
static _i zj_pool_siz;
static struct thread_task **zj_pool_stack;
static _i zj_stack_header;
static pthread_mutex_t zj_stack_header_lock;
static pthread_t zj_tid;

/**
 * ====  对外公开的接口  ==== *
 */
struct thread_pool threadpool = {
    .init = zj_pool_init,
    .add = zj_task_new,
};

static void *
zj_meta_fn(void *_ __unuse){
    pthread_detach(pthread_self());

    /*
     * 线程池中的线程程默认不接受 cancel
     * 若有需求，在传入的工作函数中更改属性即可
     */
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);

    /* 线程任务桩 */
    struct thread_task *self_task = malloc(sizeof(struct thread_task));
    if(nil == self_task){
        fprintf(stderr, "%s", strerror(errno));
        exit(1);
    }

    self_task->fn = nil;

    if(0 != pthread_cond_init(&(self_task->cond_var), nil)){
        fprintf(stderr, "%s", strerror(errno));
        exit(1);
    }

    if(0 != pthread_mutex_init(&(self_task->cond_lock), nil)){
        fprintf(stderr, "%s", strerror(errno));
        exit(1);
    }

loop: pthread_mutex_lock(&zj_stack_header_lock);

    if(zj_stack_header < (zj_pool_siz - 1)){
        zj_pool_stack[++zj_stack_header] = self_task;
        pthread_mutex_unlock(&zj_stack_header_lock);

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
        pthread_mutex_unlock(&zj_stack_header_lock);
    }

    /* 清理资源占用 */
    pthread_cond_destroy(&(self_task->cond_var));
    free(self_task);

    pthread_exit(nil);
}

/*
 * @param: siz 当前线程池初始化成功后会启动的线程数量
 * @return: 成功返回 0，失败返回负数
 */
static Error *
zj_pool_init(_i siz){
    pthread_mutexattr_t zMutexAttr;

    /*
     * 创建线程池时，先销毁旧锁，再重新初始化之，
     * 防止 fork 之后，在子进程中重建线程池时，形成死锁
     * 锁属性设置为 PTHREAD_MUTEX_NORMAL：不允许同一线程重复取锁
     */
    pthread_mutex_destroy(&zj_stack_header_lock);

    pthread_mutexattr_init(&zMutexAttr);
    pthread_mutexattr_settype(&zMutexAttr, PTHREAD_MUTEX_NORMAL);

    pthread_mutex_init(&zj_stack_header_lock, &zMutexAttr);

    pthread_mutexattr_destroy(&zMutexAttr);

    /* 允许同时处于空闲状态的线程数量，即常备线程数量 */
    zj_pool_siz = siz;

    /*
     * 必须动态初始化为 -1
     * 否则子进程继承父进程的栈索引，将带来异常
     */
    zj_stack_header = -1;

    /* 线程池栈结构空间 */
    if(nil == (zj_pool_stack = malloc(sizeof(void *) * zj_pool_siz))){
        return __err_new(errno, strerror(errno), nil);
    }

    _i i = 0;
    while(i < zj_pool_siz){
        if (0 != pthread_create(&zj_tid, nil, zj_meta_fn, nil)) {
            return __err_new(errno, strerror(errno), nil);
        }
        ++i;
    }

    return nil;
}


/*
 * 线程池容量不足时，自动扩容
 * 空闲线程过多时，会自动缩容
 */
static void
zj_task_new(void * (* fn) (void *), void *fn_param){
    pthread_mutex_lock(&zj_stack_header_lock);

    while(0 > zj_stack_header){
        pthread_mutex_unlock(&zj_stack_header_lock);
        pthread_create(&zj_tid, nil, zj_meta_fn, nil);
        pthread_mutex_lock(&zj_stack_header_lock);
    }

    pthread_mutex_lock(&zj_pool_stack[zj_stack_header]->cond_lock);

    zj_pool_stack[zj_stack_header]->fn = fn;
    zj_pool_stack[zj_stack_header]->p_param = fn_param;

    pthread_mutex_unlock(&zj_pool_stack[zj_stack_header]->cond_lock);
    pthread_cond_signal(&(zj_pool_stack[zj_stack_header]->cond_var));

    zj_stack_header--;
    pthread_mutex_unlock(&zj_stack_header_lock);
}
