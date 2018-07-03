#include "utils.h"
#include "bus.h"

#include <stdio.h>
#include <stdlib.h>

static Error * new(nng_socket *sock);
static Error * listen(const char *self_id, nng_socket *sock);
static Error * dial(nng_socket sock, const char *remote_id);

static Error * send(nng_socket sock, void *data, size_t data_len);
static Error * recv(nng_socket sock, Source *s);

struct Bus bus = {
    .new = new,
    .listen = listen,
    .dial = dial,
    .send = send,
    .recv = recv,
};

//@param sock[out]: created handler
static Error *
new(nng_socket *sock){
    _i e = nng_bus0_open(sock);
    if (0 == e) {
        return nil;
    } else {
        return __err_new(e, nng_strerror(e), nil);
    }
}

//@param self_id[in]: handler name
//@param sock[in]:
static Error *
listen(const char *self_id, nng_socket *sock){
    _i eno;
    nng_listener l;

    eno = nng_listener_create(&l, *sock, self_id);
    if (0 != eno) {
        return __err_new(eno, nng_strerror(eno), nil);
    }

    nng_listener_setopt_int(l, NNG_OPT_RECVBUF, 8192);
    //nng_listener_setopt_int(l, NNG_OPT_SENDBUF, 8192);

    eno = nng_listener_start(l, 0);
    if (0 != eno) {
        return __err_new(eno, nng_strerror(eno), nil);
    }

    return nil;
}

//@param id[in]: handler name
//@param sock[in]:
static Error *
dial(nng_socket sock, const char *remote_id){
    _i e = nng_dial(sock, remote_id, NULL, 0);
    if (0 == e) {
        return nil;
    } else {
        return __err_new(e, nng_strerror(e), nil);
    }
}

//@param sock[in]: handler
//@param data[in]: data to send
//@param data_len[in]: len of data
static Error *
send(nng_socket sock, void *data, size_t data_len){
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
recv(nng_socket sock, Source *s){
    _i e;
    if(nil == s){
        //only for info purpose!
        char buf[1];
        size_t len = 1;
        e = nng_recv(sock, &buf, &len, 0);
        if (0 != e) {
            return __err_new(e, nng_strerror(e), nil);
        }
    } else {
        e = nng_recv(sock, &s->data, &s->dsiz, NNG_FLAG_ALLOC);
        if (0 == e) {
            s->drop = utils.nng_drop;
        } else {
            return __err_new(e, nng_strerror(e), nil);
        }
    }

    return nil;
}
