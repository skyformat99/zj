#include "utils.c"
#include "threadpool.c"

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

_i n_job;
_i n_fini;

void *
add_one(void *_ __unuse){
    //__info("_");

    pthread_mutex_lock(&mlock);
    ++n_fini;
    pthread_mutex_unlock(&mlock);
    if(n_fini == n_job){
        pthread_cond_signal(&cond);
    }

    return nil;
}

_i
main(void){
    _i i;

    n_job = 1000;
    n_fini = 0;
    for(i = 0; i < n_job; ++i){
        threadpool.addjob(add_one, nil);
    }
    pthread_mutex_lock(&mlock);
    while(n_fini < n_job){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);
    So(n_job, n_fini);

    // 2000 jobs
    n_job *= 2;
    n_fini = 0;
    for(i = 0; i < n_job; ++i){
        threadpool.addjob(add_one, nil);
    }
    pthread_mutex_lock(&mlock);
    while(n_fini < n_job){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);
    So(n_job, n_fini);

    // 4000 jobs
    n_job *= 2;
    n_fini = 0;
    for(i = 0; i < n_job; ++i){
        threadpool.addjob(add_one, nil);
    }
    pthread_mutex_lock(&mlock);
    while(n_fini < n_job){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);
    So(n_job, n_fini);

    // 8000 jobs
    n_job *= 2;
    n_fini = 0;
    for(i = 0; i < n_job; ++i){
        threadpool.addjob(add_one, nil);
    }
    pthread_mutex_lock(&mlock);
    while(n_fini < n_job){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);
    So(n_job, n_fini);

    // 16000 jobs
    n_job *= 2;
    n_fini = 0;
    for(i = 0; i < n_job; ++i){
        threadpool.addjob(add_one, nil);
    }
    pthread_mutex_lock(&mlock);
    while(n_fini < n_job){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);
    So(n_job, n_fini);

    // 32000 jobs
    n_job *= 2;
    n_fini = 0;
    for(i = 0; i < n_job; ++i){
        threadpool.addjob(add_one, nil);
    }
    pthread_mutex_lock(&mlock);
    while(n_fini < n_job){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);
    So(n_job, n_fini);

    // 64000 jobs
    n_job *= 2;
    n_fini = 0;
    for(i = 0; i < n_job; ++i){
        threadpool.addjob(add_one, nil);
    }
    pthread_mutex_lock(&mlock);
    while(n_fini < n_job){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);
    So(n_job, n_fini);

    // 128000 jobs
    n_job *= 2;
    n_fini = 0;
    for(i = 0; i < n_job; ++i){
        threadpool.addjob(add_one, nil);
    }
    pthread_mutex_lock(&mlock);
    while(n_fini < n_job){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);
    So(n_job, n_fini);

    // 256000 jobs
    n_job *= 2;
    n_fini = 0;
    for(i = 0; i < n_job; ++i){
        threadpool.addjob(add_one, nil);
    }
    pthread_mutex_lock(&mlock);
    while(n_fini < n_job){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);
    So(n_job, n_fini);

    // 512000 jobs
    n_job *= 2;
    n_fini = 0;
    for(i = 0; i < n_job; ++i){
        threadpool.addjob(add_one, nil);
    }
    pthread_mutex_lock(&mlock);
    while(n_fini < n_job){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);
    So(n_job, n_fini);

    // 1024000 jobs
    n_job *= 2;
    n_fini = 0;
    for(i = 0; i < n_job; ++i){
        threadpool.addjob(add_one, nil);
    }
    pthread_mutex_lock(&mlock);
    while(n_fini < n_job){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);
    So(n_job, n_fini);
}
