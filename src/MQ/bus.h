#ifndef _BUS_H
#define _BUS_H

#include "nng/nng.h"
#include "nng/protocol/bus0/bus.h"

#include "utils.h"

struct Bus{
    Error * (*new) (nng_socket *sock) __mustuse;
    Error * (*listen) (const char *self_id, nng_socket *sock) __prm_nonnull __mustuse;
    Error * (*dial) (nng_socket sock, const char *remote_id) __prm_nonnull __mustuse;
    Error * (*send) (nng_socket, void *, size_t) __prm_nonnull __mustuse;
    Error * (*recv) (nng_socket, Source *) __mustuse;
};

struct Bus bus;

#endif //_BUS_H
