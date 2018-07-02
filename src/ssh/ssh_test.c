#include "ssh.c"

#include "utils.c"
#include "threadpool.c"
#include <time.h>

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

#define __threads_total 200
_i cnter;

static void *
thread_safe_checker(void *_ __unuse){
    //async return, unless we waiting exit_status...
    error_t *e = ssh.exec("sleep 20", nil, nil, "localhost", 22, _UNIT_TEST_USER, 10);
    __clean_errchain(e);

    pthread_mutex_lock(&mlock);
    if(__threads_total == ++cnter){
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mlock);

    return nil;
}

error_t *e = nil;
_i exit_status = 0;
source_t cmdout = {nil, 0, nil};

struct timespec tsp;
time_t now;

void
many_threads_safe(void){
    _i i = 0;
    while(i < __threads_total){
        threadpool.addjob(thread_safe_checker, nil);
        ++i;
    }

    now = time(nil);
    tsp.tv_sec = now + 60;
    tsp.tv_nsec = 0;
    pthread_mutex_lock(&mlock);
    if(__threads_total > cnter){
        i = pthread_cond_timedwait(&cond, &mlock, &tsp);
    }
    pthread_mutex_unlock(&mlock);

    So(0, i);
    So(__threads_total, cnter);
}

void
success_without_output(void){
    e = ssh.exec("ls", &exit_status, nil, "localhost", 22, _UNIT_TEST_USER, 3);
    if(nil, e) {
        __display_and_clean(e);
    }
    So(0, exit_status);
}

void
fail_without_output(void){
    e = ssh.exec("ls /root/|", &exit_status, nil, "localhost", 22, _UNIT_TEST_USER, 3);
    if(nil, e) {
        __display_and_clean(e);
    }
    SoN(0, exit_status);
}

void
success_with_output(void){
    e = ssh.exec("ls", &exit_status, &cmdout, "localhost", 22, _UNIT_TEST_USER, 3);
    if(nil, e) {
        __display_and_clean(e);
    }
    So(0, exit_status);
    SoN(nil, cmdout.data);
    //printf("||--> dsiz: %ld, data: %s\n", cmdout.dsiz, (char *)cmdout.data);
    So(1 + strlen(cmdout.data), cmdout.dsiz);
    cmdout.drop(&cmdout);
}

void
fail_with_output(void){
    e = ssh.exec("ls /root/|", &exit_status, &cmdout, "localhost", 22, _UNIT_TEST_USER, 3);
    if(nil, e) {
        __display_and_clean(e);
    }
    SoN(0, exit_status);
    SoN(nil, cmdout.data);
    //printf("||--> dsiz: %ld, data: %s\n", cmdout.dsiz, (char *)cmdout.data);
    So(1 + strlen(cmdout.data), cmdout.dsiz);
    cmdout.drop(&cmdout);
}

_i
main(void){
    many_threads_safe();

    success_without_output();
    fail_without_output();
    success_with_output();
    fail_with_output();
}
