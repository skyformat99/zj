#ifndef _ZJ_SURVEY_H
#define _ZJ_SURVEY_H

#include "nng/nng.h"
#include "nng/protocol/bus0/bus.h"

#include "zj_common.h"

struct zj_bus{
    Error * (*new) (nng_socket *sock) __mustuse;
    Error * (*listen) (const char *self_id, nng_socket *sock) __prm_nonnull __mustuse;
    Error * (*dial) (nng_socket sock, const char *remote_id) __prm_nonnull __mustuse;
    Error * (*send) (nng_socket, void *, size_t) __prm_nonnull __mustuse;
    Error * (*recv) (nng_socket, void **, size_t *) __mustuse;
};

struct zj_bus zjbus;

#endif //_ZJ_SURVEY_H
