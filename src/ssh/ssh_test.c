#include "ssh.c"

#include "utils.c"
#include "threadpool.c"
#include "convey.c"
#include <time.h>

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

#define __threads_total 20
_i cnter;

static void *
thread_safe_checker(void *_ __unuse){
    //async return, unless we waiting exit_status...
    error_t *e = ssh.exec("sleep 20", nil, nil, "localhost", 22, UNIT_TEST_USER, 10);
    if(nil != e){
        __display_and_clean(e);
    }

    pthread_mutex_lock(&mlock);
    if(__threads_total == ++cnter){
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mlock);

    return nil;
}

Main({
    Test("ssh cli tests", {
        error_t *e = nil;
        _i exit_status = 0;
        source_t cmdout;
        cmdout.data = nil;
        cmdout.dsiz = 0;
        cmdout.drop = nil;

        struct timespec tsp;
        time_t now;

        Convey("thread safe", {
            _i i = 0;
            while(i < __threads_total){
                threadpool.addjob(thread_safe_checker, nil);
                ++i;
            }

            now = time(nil);
            tsp.tv_sec = now + 5;
            tsp.tv_nsec = 0;
            pthread_mutex_lock(&mlock);
            if(__threads_total > cnter){
                i = pthread_cond_timedwait(&cond, &mlock, &tsp);
            }
            pthread_mutex_unlock(&mlock);

            So(0 == i);
            So(__threads_total == cnter);
        });

        Convey("no output", {
            e = ssh.exec("ls", &exit_status, nil, "localhost", 22, UNIT_TEST_USER, 3);
            if(nil != e) {
                __display_and_clean(e);
            }

            So(nil == e);
            So(0 == exit_status);
        });

        Convey("no output, must fail", {
            e = ssh.exec("ls /root/_", &exit_status, nil, "localhost", 22, UNIT_TEST_USER, 3);
            if(nil != e) {
                __display_and_clean(e);
            }

            So(nil == e);
            So(0 != exit_status);
        });

        Convey("have output", {
            e = ssh.exec("ls", &exit_status, &cmdout, "localhost", 22, UNIT_TEST_USER, 3);
            if(nil != e) {
                __display_and_clean(e);
            }

            So(nil == e);
            So(0 == exit_status);
            So(nil != cmdout.data);
            printf("||--> dsiz: %d, data: %s\n", cmdout.dsiz, cmdout.data);
            So(strlen(cmdout.data) == cmdout.dsiz);
            cmdout.drop(&cmdout);
        });

        Convey("have output, must fail", {
            e = ssh.exec("ls /root/_", &exit_status, &cmdout, "localhost", 22, UNIT_TEST_USER, 3);
            if(nil != e) {
                __display_and_clean(e);
            }

            So(nil == e);
            So(0 != exit_status);
            So(nil != cmdout.data);
            printf("||--> dsiz: %d, data: %s\n", cmdout.dsiz, cmdout.data);
            So(strlen(cmdout.data) == cmdout.dsiz);
            cmdout.drop(&cmdout);
        });
    });
})
