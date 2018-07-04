#ifndef _HTTPCLI_H
#define _HTTPCLI_H

#include "utils.h"

struct HttpCli{
    Error *(*get) (const char *, Source *, _i *) __prm_nonnull __mustuse;
    Error *(*post) (const char *, Source *, _i *) __prm_nonnull __mustuse;
    Error *(*req) (const char *, const char *, Source *, _i *) __prm_nonnull __mustuse;
};

struct HttpCli httpcli;

#endif //_HTTPCLI_H
