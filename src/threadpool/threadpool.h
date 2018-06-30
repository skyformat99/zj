#ifndef THREADPOOL_H
#define THREADPOOL_H

struct thread_pool {
    void (* addjob) (void * (*) (void *), void *);
};

struct thread_pool threadpool;

#endif
