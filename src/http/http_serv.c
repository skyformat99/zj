#include "utils.h"
#include "http_serv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//will be registed by webserv modules' __init func...
static struct http_serv server = {
    .srv = nil,
    .url = nil,
    .hdr = nil,
    .mlock = PTHREAD_MUTEX_INITIALIZER,
};

__final static void
http_serv_clean(void){
    if(nil != server.hdr) {
        pthread_mutex_lock(&server.mlock);

        struct http_serv_hdr *p, *pcur;
        p = server.hdr;
        pcur = p;
        while(nil !=  p){
            p = p->next;
            free(pcur);
            pcur = p;
        }

        pthread_mutex_unlock(&server.mlock);
    }

    //If the server is subsequently deallocated,
    //any of its resources will also be deallocated.
    //So, need NOT to care more details.
    if(nil != server.srv) nng_http_server_release(server.srv);
}

//@param urlstr[in]: http://[::1]:9000
static error_t *
http_start_server(const char *urlstr){
    _i rv;
    if(0 != (rv = nng_url_parse(&server.url, urlstr))){
        return __err_new(rv, nng_strerror(rv), nil);
    }
    if(0 != (rv = nng_http_server_hold(&server.srv, server.url))){
        return __err_new(rv, nng_strerror(rv), nil);
    }

    struct http_serv_hdr *p = server.hdr;
    while(nil != p){
        if(0 != (rv = nng_http_server_add_handler(server.srv, p->nnghdr))){
            return __err_new(rv, nng_strerror(rv), nil);
        }
        p = p->next;
    }

    if(0 != (rv = nng_http_server_start(server.srv))){
        return __err_new(rv, nng_strerror(rv), nil);
    }

    return nil;
}

//@method: "POST"/"GET"/nil, if nil, will accept all methods
error_t *
http_serv_hdr_register(const char *path, void (*cb) (nng_aio *prm), const char *method){
    _i rv;
    pthread_mutex_lock(&server.mlock);

    struct http_serv_hdr *p = server.hdr;
    if(nil == p){
        p = __alloc(sizeof(struct http_serv_hdr));
        server.hdr = p;
    } else {
        while(nil != p->next){
            p = p->next;
        }
        p->next = __alloc(sizeof(struct http_serv_hdr));
        p = p->next;
    }
    p->next = nil;

    pthread_mutex_unlock(&server.mlock);

    if(0 == (rv = nng_http_handler_alloc(&p->nnghdr, path, cb))
            && 0 == (rv = nng_http_handler_set_method(p->nnghdr, method))){
        return nil;
    } else {
        return __err_new(rv, nng_strerror(rv), nil);
    }
}
