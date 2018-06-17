#ifndef _ZJ_SSH_H
#define _ZJ_SSH_H

#include <stdlib.h>
#include "libssh/callbacks.h"

#include "zj_common.h"

struct zj_ssh{
	Error * (* exec)(char *, _i *, char **, size_t *, char *, _i, char *, time_t) __mustuse;
	Error * (* exec_default)(char *, char *, _i, char *) __mustuse;
};

struct zj_ssh zjssh;

#endif
