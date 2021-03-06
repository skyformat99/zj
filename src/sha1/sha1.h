#ifndef _SHA1_H
#define _SHA1_H

#include "utils.h"
#include <stdint.h>

typedef struct {
    uint32_t digest[5]; // resulting digest
    uint64_t len;       // length in bits
    uint8_t  blk[64];   // message block
    int      idx;       // index of next byte in block
} nng_sha1_ctx;

struct SHA1{
    void (*init) (nng_sha1_ctx *);
    void (*update) (nng_sha1_ctx *, const void *, size_t);
    void (*final) (nng_sha1_ctx *, uint8_t[20]);

    void (*once) (const void *, size_t, uint8_t[20]);

    Error *(*gen) (void *, size_t, char[41]) __mustuse;
    Error *(*file) (char *, char res[41]) __mustuse;
};

struct SHA1 sha1;

#endif //_SHA1_H
