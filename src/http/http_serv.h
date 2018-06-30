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
