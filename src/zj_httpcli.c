#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nng/nng.h"
#include "nng/supplemental/http/http.h"

#include "zj_httpcli.h"

#include "zj_os_target.h"
#ifdef _ZJ_OS_FREEBSD
#include <limits.h>
#endif

__prm_nonnull static Error *
zj_http_get(const char *urlstr, void **body, size_t *bsiz, _i *status_code);

__prm_nonnull static Error *
zj_http_post(const char *urlstr, void **body, size_t *bsiz, _i *status_code);

__prm_nonnull static Error *
zj_http_req(const char *urlstr, const char *method, void **body, size_t *bsiz, _i *status_code);

struct zj_httpcli zjhttpcli = {
    .get = zj_http_get,
    .post = zj_http_post,
    .req = zj_http_req,
};

typedef struct{
    nng_url *url;
    nng_http_req *req;
    nng_http_res *res;

    nng_aio *        aio;
    nng_http_client *cli;
    nng_http_conn *  conn;
}zj_http_cli_flow;

static void
zj_http_cli_flow_clean(zj_http_cli_flow *c){
    if(nil == c){ return; }

    if(nil != c->url){ nng_url_free(c->url); }
    if(nil != c->req){ nng_http_req_free(c->req); }
    if(nil != c->res){ nng_http_res_free(c->res); }

    if(nil != c->aio){ nng_aio_free(c->aio); }
    if(nil != c->conn){ nng_http_conn_close(c->conn); }
    if(nil != c->cli){ nng_http_client_free(c->cli); }
}

//@param urlstr[in]:
//@param method[in]:GET or POST
//@param body[in, out]: in and out if POST, only out if GET
//@param bsiz[in, out]: size of body
//@param status_code[out]:http status, 200/400/500...
static Error *
zj_http_req(const char *urlstr, const char *method,
        void **body, size_t *bsiz, _i *status_code){
    __drop(zj_http_cli_flow_clean) zj_http_cli_flow cl = { nil, nil, nil, nil, nil, nil };
    *status_code = -1;
    _i rv;

    if((0 != (rv = nng_url_parse(&cl.url, urlstr)))
            || 0 != ((rv = nng_http_req_alloc(&cl.req, cl.url)))
            || 0 != ((rv = nng_http_res_alloc(&cl.res)))
            || 0 != (rv = nng_http_req_set_method(cl.req, method))){
        goto fail;
    }

    if (0 != ((rv = nng_aio_alloc(&cl.aio, nil, nil)))
            || (0 != (rv = nng_http_client_alloc(&cl.cli, cl.url)))){
        goto fail;
    }

    nng_http_client_connect(cl.cli, cl.aio);
    nng_aio_wait(cl.aio);
    if (0 != (rv = nng_aio_result(cl.aio))){
        goto fail;
    }

    cl.conn = nng_aio_get_output(cl.aio, 0);

    if(nil != *body && 0 == strcmp("POST", method)){
        if(0 != (rv = nng_http_req_set_data(cl.req, *body, *bsiz))){
            goto fail;
        }
    }

    nng_http_conn_write_req(cl.conn, cl.req, cl.aio);
    nng_aio_wait(cl.aio);
    if (0 != (rv = nng_aio_result(cl.aio))){
        goto fail;
    }

    nng_http_conn_read_res(cl.conn, cl.res, cl.aio);
    nng_aio_wait(cl.aio);
    if (0 != (rv = nng_aio_result(cl.aio))) {
        goto fail;
    }

    *bsiz = 0;
    *status_code = nng_http_res_get_status(cl.res);
    const char *ptr = nng_http_res_get_header(cl.res, "Content-Length");
    if(nil != ptr) {
        *bsiz = strtol(ptr, nil, 10);
    }

    if (0 < *bsiz) {
        nng_iov iov;
        *body = nng_alloc(*bsiz);
        iov.iov_buf = *body;
        iov.iov_len = *bsiz;
        nng_aio_set_iov(cl.aio, 1, &iov);
        nng_http_conn_read_all(cl.conn, cl.aio);
        nng_aio_wait(cl.aio);
        if (0 != (rv = nng_aio_result(cl.aio))) {
            nng_free(*body, *bsiz);
            *body = nil;
            *bsiz = 0;
            goto fail;
        }
    }

    return nil;
fail:
    return __err_new(rv, nng_strerror(rv), nil);
}

static Error *
zj_http_get(const char *urlstr, void **body, size_t *bsiz, _i *status_code){
    return zj_http_req(urlstr, "GET", body, bsiz, status_code);
}

static Error *
zj_http_post(const char *urlstr, void **body, size_t *bsiz, _i *status_code){
    return zj_http_req(urlstr, "POST", body, bsiz, status_code);
}


#ifdef _ZJ_UNIT_TEST
#include "convey.c"
#include <string.h>

Main({
    Test("nng http client tests", {
        Error *e = nil;

        const char *urlstr = "http://[::1]:30000/test?name=zjms";
        const char *urlstr_bad = "http://www.163.com?~";
        const char *body = "POST";
        size_t bsiz = strlen(body);
        _i status_code = -1;

        Convey("http GET test", {
            Convey("200 test", {
                e = zjhttpcli.get(urlstr, &body, &bsiz, &status_code);
                if(nil != e){
                    __display_and_clean(e);
                }

                So(nil == e);
                So(0 == memcmp("!zjms", body, bsiz));
                So(200 == status_code);
            });

            Convey("bad req test", {
                e = zjhttpcli.get(urlstr_bad, &body, &bsiz, &status_code);
                if(nil != e){
                    __display_and_clean(e);
                }

                So(nil == e);
                So(200 != status_code);
            });
        });

        Convey("http POST test", {
            Convey("200 test", {
                e = zjhttpcli.post(urlstr, &body, &bsiz, &status_code);
                if(nil != e){
                    __display_and_clean(e);
                }

                So(nil == e);
                So(0 == memcmp("!zjmsPOST", body, bsiz));
                So(200 == status_code);
            });

            Convey("bad req test", {
                e = zjhttpcli.post(urlstr_bad, &body, &bsiz, &status_code);
                if(nil != e){
                    __display_and_clean(e);
                }

                So(nil == e);
                So(200 != status_code);
            });
        });
    });
})

#endif
