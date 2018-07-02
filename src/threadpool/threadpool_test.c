#include "utils.c"
#include "threadpool.c"

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

_i n_job, n_fini, i;

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

void
preasure_1k_jobs(void){
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
}

void
preasure_2k_jobs(void){
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

void
preasure_4k_jobs(void){
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

void
preasure_8k_jobs(void){
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

void
preasure_16k_jobs(void){
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

void
preasure_32k_jobs(void){
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

void
preasure_64k_jobs(void){
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

void
preasure_128k_jobs(void){
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

void
preasure_256k_jobs(void){
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

void
preasure_512k_jobs(void){
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

void
preasure_1024k_jobs(void){
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


_i
main(void){
    preasure_1k_jobs();
    preasure_2k_jobs();
    preasure_4k_jobs();
    preasure_8k_jobs();
    preasure_16k_jobs();
    preasure_32k_jobs();
    preasure_64k_jobs();
    preasure_128k_jobs();
    //preasure_256k_jobs();
    //preasure_512k_jobs();
    //preasure_1024k_jobs();
}
