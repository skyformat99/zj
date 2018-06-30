#ifndef Z_HTTP_SERV_H
#define Z_HTTP_SERV_H

#include <pthread.h>
#include "nng/nng.h"
#include "nng/supplemental/http/http.h"

struct http_serv_hdr{
    nng_http_handler *nnghdr; //[out]
    struct http_serv_hdr *next; //[in]
};

struct http_serv{
    nng_http_server * srv;

    nng_url *url;
    struct http_serv_hdr *hdr;

    pthread_mutex_t mlock;
};

#define __http_serv_max_body_siz 20 * 1024 * 1024 //20MB

//can only deal with POST's body
//@param hdr_name: real func_name to register to http_server
//@param hdr_inner_worker: _i body_add_1(void *reqbody, size_t reqbody_siz, source_t *resp),
//        use this worker to deal with http_req_body
#define __gen_http_hdr(hdr_name, hdr_inner_worker)\
        static void\
        hdr_name(nng_aio *aio){\
            _i rv;\
        \
            source_t req_s = {\
                .data = nil,\
                .dsiz = 0,\
                .drop = utils.non_drop,\
            };\
        \
            source_t resp_s = {\
                .data = nil,\
                .dsiz = 0,\
                .drop = utils.non_drop,\
            };\
        \
            nng_http_res *res;\
            if(0 != (rv = nng_http_res_alloc(&res))){\
                __fatal(nng_strerror(rv));\
            }\
        \
            nng_http_req *req = nng_aio_get_input(aio, 0);\
            nng_http_conn *conn = nng_aio_get_input(aio, 2);\
        \
            const char *plen = nng_http_req_get_header(req, "Content-Length");\
            if(nil != plen) {\
                if (0 < (req_s.dsiz = strtol(plen, nil, 10)) && __http_serv_max_body_siz > req_s.dsiz){\
                    nng_aio *na;\
                    if(0 != (rv = nng_aio_alloc(&na, nil, nil))){\
                        __fatal(nng_strerror(rv));\
                    }\
        \
                    req_s.data = nng_alloc(req_s.dsiz);\
                    req_s.drop = utils.nng_drop;\
        \
                    nng_iov iov;\
                    iov.iov_buf = req_s.data;\
                    iov.iov_len = req_s.dsiz;\
        \
                    nng_aio_set_iov(na, 1, &iov);\
                    nng_http_conn_read_all(conn, na);\
                    nng_aio_wait(na);\
        \
                    if (0 == (rv = nng_aio_result(na))) {\
                        if(0 != (rv = nng_http_res_set_status(res, hdr_inner_worker(req_s.data, req_s.dsiz, &resp_s)))){\
                            __fatal(nng_strerror(rv));\
                        }\
                    } else {\
                        if(0 != (rv = nng_http_res_set_status(res, 500))){\
                            __fatal(nng_strerror(rv));\
                        }\
        \
                        resp_s.data = "server error";\
                        resp_s.dsiz = sizeof("server error") - 1;\
                        resp_s.drop = utils.non_drop;\
                    }\
        \
                    nng_aio_free(na);\
                } else {\
                    if(0 != (rv = nng_http_res_set_status(res, 400))){\
                        __fatal(nng_strerror(rv));\
                    }\
        \
                    resp_s.data = "Content-Length invalid";\
                    resp_s.dsiz = sizeof("Content-Length invalid") - 1;\
                    resp_s.drop = utils.non_drop;\
                }\
            }\
        \
            if(0 != (rv = nng_http_res_copy_data(res, resp_s.data, resp_s.dsiz))){\
                __fatal(nng_strerror(rv));\
            }\
        \
            req_s.drop(&req_s);\
            resp_s.drop(&resp_s);\
        \
            if(0 != (rv = nng_aio_set_output(aio, 0, res))){\
                __info(nng_strerror(rv));\
                nng_http_conn_close(conn);\
            }\
        \
            nng_aio_finish(aio, 0);\
        }

#endif //Z_HTTP_SERV_H
