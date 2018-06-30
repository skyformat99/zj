#include "http_serv.c"

#include "http_cli.c"
#include "utils.c"
#include "threadpool.c"

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

//@param req[in]:
//@param resp[out]:
//#return http_status_code
_i
body_add_one_worker(void *reqbody, size_t reqbody_siz __unuse, source_t *resp){
    errno = 0;
    _i v = strtol(reqbody, nil, 10);
    if (0 == errno){
        resp->data = __malloc(16);
        resp->dsiz = sprintf(resp->data, "%d", ++v);
        resp->drop = utils.sys_drop;
    } else {
        resp->data = "body invalid";
        resp->dsiz = sizeof("body invalid") - 1;
        resp->drop = utils.non_drop;

        return 400;
    }

    return 200;
}

_i
echo_worker(void *reqbody, size_t reqbody_siz, source_t *resp){
    resp->data = reqbody;
    resp->dsiz = reqbody_siz;
    resp->drop = utils.non_drop;

    return 200;
}

__gen_http_hdr(body_add_one, body_add_one_worker);
__gen_http_hdr(echo, echo_worker);

__init static void
handler_register(void){
    //"POST"
    error_t *e = http_serv_hdr_register("/body_add_one", body_add_one, "POST");
    if(nil != e){
        __display_and_fatal(e);
    }

    //"GET" only
    e = http_serv_hdr_register("/echo", echo, "GET");
    if(nil != e){
        __display_and_fatal(e);
    }
}

#define __env__ {\
    e = nil;\
    status = -1;\
    s.data = "100";\
    s.dsiz = sizeof("100") - 1;\
    s.drop = nil;\
}

//NNG_HTTP_STATUS_BAD_REQUEST 400
//NNG_HTTP_STATUS_NOT_FOUND 404
//NNG_HTTP_STATUS_METHOD_NOT_ALLOWED 405
//NNG_HTTP_STATUS_INTERNAL_SERVER_ERROR 500
void *
thread_worker(void *info __unuse){
    error_t *e;
    static _i status;
    static source_t s;

    __env__
    if(nil != (e = httpcli.get("http://localhost:9000/body_add_one", &s, &status))){
        __display_and_fatal(e);
    }
    So(405, status);
    s.drop(&s);

    __env__
    if(nil != (e = httpcli.post("http://localhost:9000/body_add_one", &s, &status))){
        __display_and_fatal(e);
    }
    So(200, status);
    So(0, strcmp("101", s.data));
    So(sizeof("101") - 1, s.dsiz);
    So(utils.nng_drop, s.drop);
    s.drop(&s);

    //test bad request info
    __env__
    s.dsiz = 1000000000;
    if(nil != (e = httpcli.post("http://localhost:9000/body_add_one", &s, &status))){
        __display_and_fatal(e);
    }
    So(200, status);
    So(0, strcmp("101", s.data));
    So(sizeof("101") - 1, s.dsiz);
    So(utils.nng_drop, s.drop);
    s.drop(&s);

    __env__
    if(nil != (e = httpcli.post("http://localhost:9000/__not_exist__", &s, &status))){
        __display_and_fatal(e);
    }
    So(404, status);
    s.drop(&s);

    __env__
    if(nil != (e = httpcli.get("http://localhost:9000/echo", &s, &status))){
        __display_and_fatal(e);
    }
    So(200, status);
    So(0, strcmp("100", s.data));
    So(sizeof("100") - 1, s.dsiz);
    So(utils.nng_drop, s.drop);
    s.drop(&s);

    __env__
    if(nil != (e = httpcli.post("http://localhost:9000/echo", &s, &status))){
        __display_and_fatal(e);
    }
    So(405, status);
    s.drop(&s);

    pthread_mutex_lock(&mlock);
    pthread_mutex_unlock(&mlock);
    pthread_cond_signal(&cond);
    return nil;
}

_i
main(void){
    error_t *e = nil;
    static char *serv_url = "http://localhost:9000";

    e = http_start_server(serv_url);
    if(nil != e){
        __display_and_fatal(e);
    }

    pthread_mutex_lock(&mlock);
    threadpool.addjob(thread_worker, nil);

    struct timespec tsp;
    tsp.tv_sec = time(nil) + 3;
    tsp.tv_nsec = 0;
    pthread_cond_timedwait(&cond, &mlock, &tsp);
    pthread_mutex_unlock(&mlock);
}
