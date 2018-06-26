#include <stdio.h>
#include <stdlib.h>

#include "zj_bus.h"

static Error * zj_new(const char *self_id, nng_socket *sock); 
static Error * zj_dial(const char *remote_id, nng_socket sock);
static Error * zj_send(nng_socket sock, void *data, size_t data_len);
static Error * zj_recv(nng_socket sock, void **data, size_t *data_len);

struct zj_bus zjbus = {
    .new = zj_new,
    .dial = zj_dial,
    .send = zj_send,
    .recv = zj_recv,
};

//@param self_id[in]: handler name
//@param sock[out]: created handler
static Error *
zj_new(const char *self_id, nng_socket *sock){
    _i e = nng_bus0_open(sock);
    if (0 != e) {
        return __err_new(e, nng_strerror(e), nil);
    }

    e = nng_listen(*sock, self_id, NULL, 0);
    if (0 != e) {
        return __err_new(e, nng_strerror(e), nil);
    }

    return nil;
}

//@param id[in]: handler name
//@param sock[in]:
static Error *
zj_dial(const char *remote_id, nng_socket sock){
    _i e = nng_dial(sock, remote_id, NULL, 0);
    if (0 != e) {
        return __err_new(e, nng_strerror(e), nil);
    }

    return nil;
}

//@param sock[in]: handler
//@param data[in]: data to send
//@param data_len[in]: len of data
static Error *
zj_send(nng_socket sock, void *data, size_t data_len){
    _i e = nng_send(sock, data, data_len, 0);
    if (0 != e) {
        return __err_new(e, nng_strerror(e), nil);
    }
    return nil;
}

//@param sock[in]: handler
//@param data[out]: must call nng_free except given 'nil'
//@param data_len[out]: actual len of recved data
static Error *
zj_recv(nng_socket sock, void **data, size_t *data_len){
    _i e;
    if(nil == data || nil == data_len){
        //only for info purpose!
        char buf[1];
        size_t len = 1;
        e = nng_recv(sock, &buf, &len, 0);
        if (0 != e) {
            return __err_new(e, nng_strerror(e), nil);
        }
    } else {
        //must call: nng_free()
        e = nng_recv(sock, data, data_len, NNG_FLAG_ALLOC);
        if (0 != e) {
            return __err_new(e, nng_strerror(e), nil);
        }
    }

    return nil;
}


//#define _ZJ_UNIT_TEST
//#define _XOPEN_SOURCE 700
#ifdef _ZJ_UNIT_TEST
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "zj_threadpool.c"
#include "zj_utils.c"
#include "convey.c"

#define __node_total 20
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t br;

_i slave_cnter;

#define __atomic_log(__e) do{\
    pthread_mutex_lock(&mlock);\
    __display_and_clean(__e);\
    pthread_mutex_unlock(&mlock);\
}while(0)

typedef struct {
    char self_url[64];
    char remote_url[64];
} bus_info;

void *
thread_worker(void *info){
    bus_info *bi = (bus_info *)info;
    nng_socket sock;
    Error *e = zjbus.new(bi->self_url, &sock);
    if(nil != e){
        __atomic_log(e);
        exit(1);
    }

    pthread_barrier_wait(&br);

    e = zjbus.dial(bi->remote_url, sock);
    if(nil != e){
        __atomic_log(e);
        exit(1);
    }

    pthread_mutex_lock(&mlock);
    zjutils.sleep(0.1);
    e = zjbus.send(sock, "", sizeof(""));
    if(nil != e){
        __display_and_clean(e);
        exit(1);
    }
    pthread_mutex_unlock(&mlock);

    e = zjbus.recv(sock, nil, nil);
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

Main({
    Test("nng bus tests", {
        Error *e = nil;
        static char *leader_url = "tcp://[::1]:9000";

        Convey("thread_barrier mode", {
            _i eno = pthread_barrier_init(&br, nil, 1 + __node_total);
            if(0 != eno){
                __info(strerror(eno));
                exit(1);
            }

            e = threadpool.init(__node_total);
            if(nil != e){
                __atomic_log(e);
                exit(1);
            }

            nng_socket sock;
            e = zjbus.new(leader_url, &sock);
            if(nil != e){
                __atomic_log(e);
                exit(1);
            }

            char slave_url[__node_total][64];
            _i i = 0;
            while(i < __node_total){
                bus_info *bi = __malloc(sizeof(bus_info));
                snprintf(bi->self_url, 64, "tcp://[::1]:%d", 9001 + i);
                strcpy(bi->remote_url, leader_url);

                strcpy(slave_url[i], bi->self_url);

                threadpool.add(thread_worker, bi);
                ++i;
            }

            pthread_barrier_wait(&br);

            i = 0;
            while(i < __node_total){
                e = zjbus.recv(sock, nil, nil);
                if(nil != e){
                    __atomic_log(e);
                    exit(1);
                }
                ++i;
            }

            pthread_barrier_destroy(&br);
            So(__node_total == i);

            i = 0;
            while(i < __node_total){
                e = zjbus.dial(slave_url[i], sock);
                if(nil != e){
                    __atomic_log(e);
                    exit(1);
                }
                ++i;
            }

            i = 0;
            while(i < __node_total){
                e = zjbus.send(sock, "", sizeof(""));
                if(nil != e){
                    __display_and_clean(e);
                    exit(1);
                }
                ++i;
            }

            nng_close(sock);
            So(__node_total == i);
        });
    });
})
#endif
