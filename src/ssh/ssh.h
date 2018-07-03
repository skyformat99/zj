#ifndef _SSH_H
#define _SSH_H

#include "utils.h"
#include "libssh/callbacks.h"

struct SSH{
    Error * (* exec)(char *, _i *, Source *, char *, _i, char *, time_t) __mustuse;
    Error * (* exec_default)(char *, char *, _i, char *) __mustuse;
};

struct SSH ssh;

#endif //_SSH_H
