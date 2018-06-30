#ifndef BUS_H
#define BUS_H

#include "nng/nng.h"
#include "nng/protocol/bus0/bus.h"

#include "utils.h"

struct bus{
    error_t * (*new) (nng_socket *sock) __mustuse;
    error_t * (*listen) (const char *self_id, nng_socket *sock) __prm_nonnull __mustuse;
    error_t * (*dial) (nng_socket sock, const char *remote_id) __prm_nonnull __mustuse;
    error_t * (*send) (nng_socket, void *, size_t) __prm_nonnull __mustuse;
    error_t * (*recv) (nng_socket, source_t *) __mustuse;
};

struct bus bus;

#endif //BUS_H
