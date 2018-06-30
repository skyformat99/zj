#ifndef SSH_H
#define SSH_H

#include "libssh/callbacks.h"

#include "utils.h"

struct ssh{
    error_t * (* exec)(char *, _i *, source_t *, char *, _i, char *, time_t) __mustuse;
    error_t * (* exec_default)(char *, char *, _i, char *) __mustuse;
};

struct ssh ssh;

#endif
