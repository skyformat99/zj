#include "bus.c"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "utils.c"
#include "threadpool.c"

#define __node_total 20
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

_i slave_cnter = -1;

struct bus_info{
    char self_url[64];
    char remote_url[64];
};

void *
thread_worker(void *info){
    struct bus_info *bi = (struct bus_info *)info;
    nng_socket sock;
    error_t *e = bus.new(&sock);
    if(nil != e){
        __display_and_clean(e);
        exit(1);
    }

    pthread_mutex_lock(&mlock);
    while(-1 == slave_cnter){
        pthread_cond_wait(&cond, &mlock);
    }
    pthread_mutex_unlock(&mlock);

    e = bus.dial(sock, bi->remote_url);
    if(nil != e){
        __display_and_clean(e);
        exit(1);
    }

    e = bus.listen(bi->self_url, &sock);
    if(nil != e){
        __display_and_clean(e);
        exit(1);
    }

    pthread_mutex_lock(&mlock);
    utils.msleep(10); // 10msecs
    e = bus.send(sock, "", sizeof(""));
    if(nil != e){
        __display_and_clean(e);
        exit(1);
    }
    pthread_mutex_unlock(&mlock);

    e = bus.recv(sock, nil);
    if(nil != e){
        __display_and_clean(e);
        exit(1);
    }

    pthread_mutex_lock(&mlock);
    ++slave_cnter;
    pthread_mutex_unlock(&mlock);

    nng_close(sock);
    free(bi);

    return nil;
}

_i
main(void){
    error_t *e = nil;
    _i rv = 0;
    static char *leader_url = "tcp://localhost:9000";

    nng_socket sock;
    e = bus.new(&sock);
    if(nil != e){
        __display_and_clean(e);
        exit(1);
    }

    e = bus.listen(leader_url, &sock);
    if(nil != e){
        __display_and_clean(e);
        exit(1);
    }

    //timeout 100ms
    rv = nng_setopt_ms(sock, NNG_OPT_RECVTIMEO, 100);
    if(0 != rv){
        __fatal(nng_strerror(rv));
    }

    char slave_url[__node_total][64];
    _i i = 0;
    while(i < __node_total){
        struct bus_info *bi = __malloc(sizeof(struct bus_info));
        snprintf(bi->self_url, 64, "tcp://localhost:%d", 9001 + i);
        strcpy(bi->remote_url, leader_url);

        strcpy(slave_url[i], bi->self_url);

        threadpool.addjob(thread_worker, bi);
        ++i;
    }

    pthread_mutex_lock(&mlock);
    slave_cnter = 0;
    pthread_mutex_unlock(&mlock);
    pthread_cond_broadcast(&cond);

    i = 0;
    while(i < __node_total){
        e = bus.recv(sock, nil);
        if(nil != e){
            __display_and_clean(e);
            exit(1);
        }
        ++i;
    }

    So(__node_total, i);

    i = 0;
    while(i < __node_total){
        e = bus.dial(sock, slave_url[i]);
        if(nil != e){
            __display_and_clean(e);
            exit(1);
        }
        ++i;
    }

    i = 0;
    while(i < __node_total){
        e = bus.send(sock, "", sizeof(""));
        if(nil != e){
            __display_and_clean(e);
            exit(1);
        }
        ++i;
    }

    nng_close(sock);
    So(__node_total, i);
}