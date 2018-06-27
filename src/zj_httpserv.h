#include "zj_common.h"

#include <pthread.h>
#include "nng/nng.h"
#include "nng/supplemental/http/http.h"

//@field segkey[in]: url segment, such as 'name' in http://[::1]:9000?name=john
typedef	struct zj_http_serv_hdr_unit{
	char *path;

	nng_http_handler *func;
    _i nseg;
    char **segkey; //segs those needed by the func

	struct zj_http_serv_hdr_unit *next;
}zj_http_serv_hdr_unit;

typedef struct zj_http_serv_hdr{
	pthread_mutex_t mlock;
	zj_http_serv_hdr_unit *hu;
}zj_http_serv_hdr;

#define __zj_http_serv_hdr_register(__glob_hdr, __path, __func) do{\
	zj_http_serv_hdr_unit *newhdr = __malloc(sizeof(zj_http_serv_hdr_unit));\
	newhdr->next = nil;\
	newhdr->func = __func;\
	newhdr->path = __malloc(1 + strlen(__path));\
	strcpy(newhdr->path, __path);\
\
	pthread_mutex_lock(&__glob_hdr->mlock);\
	if(nil == __glob_hdr->hu){\
		__glob_hdr->hu = newhdr;\
	} else {\
		zj_http_serv_hdr_unit *p = __glob_hdr->hu;\
		while(nil != p->next){\
			p = p->next;\
		}\
		p->next = newhdr;\
	}\
	pthread_mutex_unlock(&__glob_hdr->mlock);\
}while(0)
