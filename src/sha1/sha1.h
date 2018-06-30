#ifndef Z_NNG_SHA1_H
#define Z_NNG_SHA1_H

#include <stdint.h>
#include <sys/types.h>

typedef struct {
    uint32_t digest[5]; // resulting digest
    uint64_t len;       // length in bits
    uint8_t  blk[64];   // message block
    int      idx;       // index of next byte in block
} nng_sha1_ctx;

struct nng_sha1{
    void (*init) (nng_sha1_ctx *);
    void (*update) (nng_sha1_ctx *, const void *, size_t);
    void (*final) (nng_sha1_ctx *, uint8_t[20]);
    
    void (*once) (const void *, size_t, uint8_t[20]);
    
    void (*lower_case) (void *, size_t, char[41]);
    void (*upper_case) (void *, size_t, char[41]);
};

struct nng_sha1 nngsha1;

#endif // Z_NNG_SHA1_H
