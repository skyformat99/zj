#ifndef _HTTPCLI_H
#define _HTTPCLI_H

#include <sys/types.h>
#include "utils.h"

struct http_cli{
    error_t * (*get) (const char *, source_t *, _i *);
    error_t * (*post) (const char *, source_t *, _i *);
    error_t * (*req) (const char *, const char *, source_t *, _i *);
};

#endif //_HTTPCLI_H
