#ifndef _HTTPCLI_H
#define _HTTPCLI_H

#include "utils.h"

struct HttpCli{
    Error * (*get) (const char *, Source *, _i *);
    Error * (*post) (const char *, Source *, _i *);
    Error * (*req) (const char *, const char *, Source *, _i *);
};

struct HttpCli httpcli;

#endif //_HTTPCLI_H
