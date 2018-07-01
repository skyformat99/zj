#ifndef _THREADPOOL_H
#define _THREADPOOL_H

struct thread_pool {
    void (* addjob) (void * (*) (void *), void *);
};

struct thread_pool threadpool;

#endif //_THREADPOOL_H
