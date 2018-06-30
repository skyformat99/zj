#include "http_serv.c"

#include "http_cli.c"
#include "utils.c"
#include "threadpool.c"

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static void
body_add_one(nng_aio *aio){
    _i rv;
    nng_http_res *res;

    const char *pheader;
    size_t dsiz;
    void *data;

    _li valueData;
    _i bodysiz = 0;
    char bodybuf[64];

    if(0 != (rv = nng_http_res_alloc(&res))){
        __fatal(nng_strerror(rv));
    }

    nng_http_req *req = nng_aio_get_input(aio, 0);
    //nng_http_handler *hdr = nng_aio_get_input(aio, 1);
    nng_http_conn *conn = nng_aio_get_input(aio, 2);

    if(nil != (pheader = nng_http_req_get_header(req, "Content-Length"))) {
        if (0 < (dsiz = strtol(pheader, nil, 10))){
            nng_aio *recv_aio;
            if(0 != (rv = nng_aio_alloc(&recv_aio, nil, nil))){
                bodysiz = sprintf(bodybuf, "_500");
                goto serv500;
            }

            data = nng_alloc(dsiz);
            nng_iov iov;
            iov.iov_buf = data;
            iov.iov_len = dsiz;
            nng_aio_set_iov(recv_aio, 1, &iov);
            nng_http_conn_read_all(conn, recv_aio);
            nng_aio_wait(recv_aio);
            if (0 == (rv = nng_aio_result(recv_aio))) {
                errno = 0;
                valueData = strtol("1", nil, 10);
                if (0 == errno){
                    bodysiz = sprintf(bodybuf, "%ld", ++valueData);
                    if(0 != (rv = nng_http_res_set_status(res, 200))){
                        __fatal(nng_strerror(rv));
                    }

                    nng_free(data, dsiz);
                    nng_aio_free(recv_aio);
                } else {
                    nng_free(data, dsiz);
                    nng_aio_free(recv_aio);

                    bodysiz = sprintf(bodybuf, "_402");
                    goto req400;
                }
            } else {
                nng_free(data, dsiz);
                nng_aio_free(recv_aio);

                bodysiz = sprintf(bodybuf, "_500");
                goto serv500;
            }
        } else {
            bodysiz = sprintf(bodybuf, "_403");
            goto req400;
        }
    }

    if(0){
req400:
        if(0 != (rv = nng_http_res_set_status(res, 400))){
            __fatal(nng_strerror(rv));
        }
    }

    if(0){
serv500:
        if(0 != (rv = nng_http_res_set_status(res, 500))){
            __fatal(nng_strerror(rv));
        }
    }

    //will auto set "Content-Length", and auto_free
    if(0 != (rv = nng_http_res_copy_data(res, bodybuf, bodysiz))){
        __fatal(nng_strerror(rv));
    }

    if(0 != (rv = nng_aio_set_output(aio, 0, res))){
        __info(nng_strerror(rv));
        nng_http_conn_close(conn);
    }

    nng_aio_finish(aio, 0);
}

static void
echoecho(nng_aio *aio){
    _i rv;
    nng_http_res *res;

    const char *pheader;
    size_t dsiz;
    void *data;

    if(0 != (rv = nng_http_res_alloc(&res))){
        __fatal(nng_strerror(rv));
    }

    nng_http_req *req = nng_aio_get_input(aio, 0);
    //nng_http_handler *hdr = nng_aio_get_output(prm, 1);
    nng_http_conn *conn = nng_aio_get_input(aio, 2);

    if(nil != (pheader = nng_http_req_get_header(req, "Content-Length"))) {
        if (0 < (dsiz = strtol(pheader, nil, 10))){
            nng_aio *recv_aio;
            if(0 != (rv = nng_aio_alloc(&recv_aio, nil, nil))){
                goto serv500;
            }

            data = nng_alloc(dsiz);
            nng_iov iov;
            iov.iov_buf = data;
            iov.iov_len = dsiz;
            nng_aio_set_iov(recv_aio, 1, &iov);
            nng_http_conn_read_all(conn, recv_aio);
            nng_aio_wait(recv_aio);
            if (0 == (rv = nng_aio_result(recv_aio))) {
                nng_http_res_copy_data(res, data, dsiz);
                nng_http_res_set_status(res, 200);

                nng_free(data, dsiz);
                nng_aio_free(recv_aio);
            } else {
                nng_free(data, dsiz);
                nng_aio_free(recv_aio);

                goto serv500;
            }
        } else {
            nng_http_res_set_data(res, "_400", 4);
            nng_http_res_set_status(res, 400);
        }
    }

    if(0){
serv500:
        nng_http_res_set_data(res, "_500", 4);
        nng_http_res_set_status(res, 500);
    }

    if(0 != (rv = nng_aio_set_output(aio, 0, res))){
        __info(nng_strerror(rv));
        nng_http_conn_close(conn);
    }

    nng_aio_finish(aio, 0);
}

__init static void
handler_register(void){
    //"GET" and "POST"
    error_t *e = http_serv_hdr_register("/body_add_one", body_add_one, nil);
    if(nil != e){
        __display_and_fatal(e);
    }

    //"GET" only
    e = http_serv_hdr_register("/echoecho", echoecho, "GET");
    if(nil != e){
        __display_and_fatal(e);
    }
}

#define __env__ {\
    e = nil;\
    status = -1;\
    s.data = "0";\
    s.dsiz = 1;\
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
    s.drop(&s);
    So(200, status);

    __env__
    if(nil != (e = httpcli.post("http://localhost:9000/body_add_one", &s, &status))){
        __display_and_clean(e);
        exit(1);
    }
    s.drop(&s);
    So(200, status);

    __env__
    if(nil != (e = httpcli.get("http://localhost:9000/echoecho", &s, &status))){
        __display_and_clean(e);
        exit(1);
    }
    s.drop(&s);
    So(200, status);

    __env__
    if(nil != (e = httpcli.post("http://localhost:9000/echoecho", &s, &status))){
        __display_and_clean(e);
        exit(1);
    }
    s.drop(&s);
    So(405, status);

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
