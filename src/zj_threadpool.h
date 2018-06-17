#ifndef _ZJ_THREADPOOL_H
#define _ZJ_THREADPOOL_H

#include "zj_common.h"
#include <pthread.h>
#include <semaphore.h>

struct thread_task {
    pthread_cond_t cond_var;
    char __pad[128];
    pthread_mutex_t cond_lock;

    void * (* fn) (void *);
    void *p_param;
};

struct thread_pool {
    Error *(* init) (_i, _i) __mustuse;
    void (* add) (void * (*) (void *), void *);

    sem_t *p_limit_sem;
};

struct thread_pool threadpool;
#endif
