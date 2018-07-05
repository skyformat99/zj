#include "utils.h"
#include "http_cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "nng/nng.h"
#include "nng/supplemental/http/http.h"

#ifdef _OS_FREEBSD
#include <limits.h>
#endif

#define __http_req_max_body_siz 1 * 1024 * 1024 //1MB

static Error *http_get(const char *urlstr, Source *s, _i *status_code) __mustuse;
static Error *http_post(const char *urlstr, Source *s, _i *status_code) __mustuse;
static Error *http_req(const char *urlstr, const char *method, Source *s, _i *status_code) __mustuse;

struct HttpCli httpcli = {
    .get = http_get,
    .post = http_post,
    .req = http_req,
};

struct HttpCliFlow{
    nng_url *url;
    nng_http_req *req;
    nng_http_res *res;

    nng_aio *        aio;
    nng_http_client *cli;
    nng_http_conn *  conn;
};

static void
HttpCliFlow_clean(struct HttpCliFlow *c){
    if(nil == c) return;

    if(nil != c->url) nng_url_free(c->url);
    if(nil != c->req) nng_http_req_free(c->req);
    if(nil != c->res) nng_http_res_free(c->res);

    if(nil != c->aio) nng_aio_free(c->aio);
    if(nil != c->conn) nng_http_conn_close(c->conn);
    if(nil != c->cli) nng_http_client_free(c->cli);
}

//@param urlstr[in]:
//@param method[in]:GET or POST
//@param body[in, out]: in and out if POST, only out if GET
//@param bsiz[in, out]: size of body
//@param status_code[out]:http status, 200/400/500...
static Error *
http_req(const char *urlstr, const char *method, Source *s, _i *status_code){
    __check_nil(urlstr&&method&&s&&status_code);
    __drop(HttpCliFlow_clean)
    struct HttpCliFlow cl = { nil, nil, nil, nil, nil, nil };
    _i rv = 0;

    s->drop = utils.non_drop;
    *status_code = -1;

    if((0 != (rv = nng_url_parse(&cl.url, urlstr)))
            || 0 != ((rv = nng_http_req_alloc(&cl.req, cl.url)))
            || 0 != ((rv = nng_http_res_alloc(&cl.res)))
            || 0 != (rv = nng_http_req_set_method(cl.req, method))){
        return __err_new(rv, nng_strerror(rv), nil);
    }

    if (0 != ((rv = nng_aio_alloc(&cl.aio, nil, nil)))
            || (0 != (rv = nng_http_client_alloc(&cl.cli, cl.url)))){
        return __err_new(rv, nng_strerror(rv), nil);
    }

    nng_http_client_connect(cl.cli, cl.aio);
    nng_aio_wait(cl.aio);
    if (0 != (rv = nng_aio_result(cl.aio))){
        return __err_new(rv, nng_strerror(rv), nil);
    }

    cl.conn = nng_aio_get_output(cl.aio, 0);

    if(nil != s->data){
        if(__http_req_max_body_siz < s->dsiz){
            return __err_new(-1, "data size too big", nil);
        }

        if(0 != (rv = nng_http_req_copy_data(cl.req, s->data, s->dsiz))){
            return __err_new(rv, nng_strerror(rv), nil);
        }
    }

    nng_http_conn_write_req(cl.conn, cl.req, cl.aio);
    nng_aio_wait(cl.aio);
    if (0 != (rv = nng_aio_result(cl.aio))){
        return __err_new(rv, nng_strerror(rv), nil);
    }

    nng_http_conn_read_res(cl.conn, cl.res, cl.aio);
    nng_aio_wait(cl.aio);
    if (0 != (rv = nng_aio_result(cl.aio))) {
        return __err_new(rv, nng_strerror(rv), nil);
    }

    *status_code = nng_http_res_get_status(cl.res);

    s->data = nil;
    s->dsiz = 0;

    const char *ptr = nng_http_res_get_header(cl.res, "Content-Length");
    if(nil != ptr){
        s->dsiz = strtol(ptr, nil, 10);
        if (0 < s->dsiz) {
            nng_iov iov;
            if(nil == (s->data = nng_alloc(s->dsiz))){
                return __err_new(-1, ptr, nil);
            } else {
                s->drop = utils.nng_drop;
            }

            iov.iov_buf = s->data;
            iov.iov_len = s->dsiz;
            nng_aio_set_iov(cl.aio, 1, &iov);
            nng_http_conn_read_all(cl.conn, cl.aio);
            nng_aio_wait(cl.aio);
            if (0 != (rv = nng_aio_result(cl.aio))) {
                nng_free(s->data, s->dsiz);
                s->data = nil;
                s->dsiz = 0;
                s->drop = utils.non_drop;
                return __err_new(rv, nng_strerror(rv), nil);
            }
        }
    }

    return nil;
}

static Error *
http_get(const char *urlstr, Source *s, _i *status_code){
    return http_req(urlstr, "GET", s, status_code);
}

static Error *
http_post(const char *urlstr, Source *s, _i *status_code){
    return http_req(urlstr, "POST", s, status_code);
}
