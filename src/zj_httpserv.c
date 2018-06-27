#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "zj_httpserv.h"

//@field nseg[in]: num of seg
//@field segvalue[out]: url segment, such as 'john' in http://[::1]:9000?name=john
//@field body[out]: http_req's body, nil if no req_body found
//@field bsiz[out]: size of body, 0 if nil == body
typedef struct{
    _i nseg;

    char **segvalue;
    void *body;
    size_t bsiz;
}zj_http_hdr_info;

static void
zj_http_hdr_info_init(zj_http_hdr_info *hi, _i nseg){
    hi->nseg = nseg;

    hi->segvalue = __malloc(nseg * sizeof(void *));
    hi->body = nil;
    hi->bsiz = 0;
}

static void
zj_http_hdr_info_clean(zj_http_hdr_info *hi){
    if(nil == hi){
        return;
    }

    for(; hi->nseg > 0; hi->nseg--){
        if(nil != hi->segvalue[hi->nseg - 1]){
            free(hi->segvalue[hi->nseg - 1]);
        }
    }
    free(hi->segvalue);

    if(nil != hi->body){
        free(hi->body);
    }
}

typedef struct{
    nng_url *url;
    nng_aio *aio;
    nng_http_server * srv;

    zj_http_serv_hdr *hdr;
}zj_http_serv_flow;

static void
zj_http_serv_hdr_init(zj_http_serv_hdr **h){
	*h = __malloc(sizeof(zj_http_serv_hdr));
	(*h)->hu = nil;

	_i rv = pthread_mutex_init(&(*h)->mlock, nil);
	if(0 != rv){
		__info(strerror(rv));
		exit(1);
	}
}

static void
zj_http_serv_flow_clean(zj_http_serv_flow *s){
    if(nil == s){ return; }

    if(nil != s->aio){ nng_aio_free(s->aio); }
    if(nil != s->url){ nng_url_free(s->url); }
    if(nil != s->srv){ nng_http_server_release(s->srv); }

	zj_http_serv_hdr_unit *p, *pcur;
	p = s->hdr->hu;
	pcur = p;
    while(nil !=  p){
		free(p->path);
		nng_http_handler_free(p->func);

		p = p->next;
		free(pcur);
		pcur = p;
	}

	pthread_mutex_destroy(&s->hdr->mlock);
}

Error *
zj_http_start_server(const char *urlstr){
    nng_http_server * s;
    nng_http_handler *h;

    nng_aio *aio;
    nng_url *url;

    So(nng_url_parse(&url, urlstr) == 0);
    So(nng_aio_alloc(&aio, NULL, NULL) == 0);

    So(nng_http_server_hold(&s, url) == 0);

    So(nng_http_handler_alloc_static(&h, "/home.html", doc1,
           strlen(doc1), "text/html") == 0);
    So(nng_http_server_add_handler(s, h) == 0);
    So(nng_http_server_start(s) == 0);

	return nil;
}
