#ifndef _OS_H
#define _OS_H

#include "utils.h"
#include <sys/uio.h>

struct os{
    void (*daemonize) (const char *);
    error_t *(* rm_all) (char *);

    error_t *(*set_nonblocking) (_i);
    error_t *(*set_blocking) (_i);

    error_t *(*socket_new) (const char *, const char *, _i *);
    error_t *(*ip_socket_new) (const char *, const char *, _i *) __prm_nonnull;
    error_t *(*unix_socket_new) (const char *, _i *) __prm_nonnull;

    error_t * (*listen) (_i);

    error_t *(*connect) (const char *, const char *, _i *);

    error_t *(*send) (_i, void *, size_t);
    error_t *(*sendmsg) (_i, struct iovec *, size_t);
    error_t *(*recv) (_i, void *, size_t);
    error_t *(*recvall) (_i, void *, size_t);
};

struct os os;

#endif //_OS_H
