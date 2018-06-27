#ifndef _ZJ_HTTPCLI_H
#define _ZJ_HTTPCLI_H

#include <sys/types.h>
#include "zj_common.h"

struct zj_httpcli{
	Error * (*get) (const char *, void **, size_t *, _i *);
	Error * (*post) (const char *, void **, size_t *, _i *);
	Error * (*req) (const char *, const char *, void **, size_t *, _i *);
};

#endif //_ZJ_HTTPCLI_H
