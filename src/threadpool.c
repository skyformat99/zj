#include "threadpool.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static Error *pool_init(_i, _i);
static void task_new(void * (*) (void *), void *);

/* 线程池栈结构 */
static _i pool_siz;

static struct thread_task **pool_stack;

static _i stack_header;
static pthread_mutex_t stack_header_lock;

static pthread_t tid;

/**
 * ====  对外公开的接口  ==== *
 */
struct thread_pool threadpool = {
    .init = pool_init,
    .add = task_new,
};

static void *
meta_fn(void *_ __unuse){
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

    /* 释放占用的系统全局信号量并清理资源占用 */
    sem_post(threadpool.p_limit_sem);
    pthread_cond_destroy(&(self_task->cond_var));
    free(self_task);

    pthread_exit(nil);
}

static char *limit_sem_path = nil;
static void
limit_sem_clean(void){
    sem_close(threadpool.p_limit_sem);
    sem_unlink(limit_sem_path);
}

/*
 * @param: glob_siz 系统全局线程数量上限
 * @param: siz 当前线程池初始化成功后会启动的线程数量
 * @return: 成功返回 0，失败返回负数
 */
static Error *
pool_init(_i siz, _i glob_siz){
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

    /* 允许同时处于空闲状态的线程数量，即常备线程数量 */
    pool_siz = siz;

    /*
     * 必须动态初始化为 -1
     * 否则子进程继承父进程的栈索引，将带来异常
     */
    stack_header = -1;

    /* 线程池栈结构空间 */
    if(nil == (pool_stack = malloc(sizeof(void *) * pool_siz))){
        return __err_new(errno, strerror(errno), nil);
    }

    /*
     * 主进程程调用时，
     *     glob_siz 置为正整数，会尝试清除已存在的旧文件，并新建信号量
     * 子进程中调用时，
     *     glob_siz 置为负数或 0，自动继承主进程的 handler
     */
    if(0 < glob_siz){
        char sem_name[128];
        _i n = sprintf(sem_name, "/threadpool_sem%d", getpid());
        threadpool.p_limit_sem = sem_open(sem_name, O_CREAT|O_EXCL, 0700, glob_siz);
        if(SEM_FAILED == threadpool.p_limit_sem){
            if(EEXIST != errno){
                return __err_new(errno, strerror(errno), nil);
            }
        } else {
            limit_sem_path = __malloc(1 + n);
            strcpy(limit_sem_path, sem_name);

            atexit(limit_sem_clean);
        }
    }

    _i i = 0;
    while(i < pool_siz){
        if (0 != sem_trywait(threadpool.p_limit_sem)) {
            return __err_new(errno, strerror(errno), nil);
        } else {
            if (0 != pthread_create(&tid, nil, meta_fn, nil)) {
                return __err_new(errno, strerror(errno), nil);
            }
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
task_new(void * (* fn) (void *), void *fn_param){
    pthread_mutex_lock(&stack_header_lock);

    while(0 > stack_header){
        pthread_mutex_unlock(&stack_header_lock);

        /* 不能超过系统全局范围线程总数限制 */
        sem_wait(threadpool.p_limit_sem);

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
