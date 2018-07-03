#ifndef _THREADPOOL_H
#define _THREADPOOL_H

struct ThreadPool{
    void (* addjob) (void * (*) (void *), void *);
};

struct ThreadPool threadpool;

#endif //_THREADPOOL_H
