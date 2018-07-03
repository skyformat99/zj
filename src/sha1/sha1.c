/**
 * NOTE:
 * borrow from nng-1.0.0
 */
#include "sha1.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

static void nng_sha1_init(nng_sha1_ctx *);
static void nng_sha1_update(nng_sha1_ctx *, const void *, size_t);
static void nng_sha1_final(nng_sha1_ctx *, uint8_t[20]);

static void nng_sha1(const void *, size_t, uint8_t[20]);

static void sha1_lower_case(void *, size_t, char[41]);
static Error * sha1_file_lower_case(char *filepath, char res[41]);

struct SHA1 sha1 = {
    .init = nng_sha1_init,
    .update = nng_sha1_update,
    .final = nng_sha1_final,

    .once = nng_sha1,

    .gen = sha1_lower_case,
    .file = sha1_file_lower_case,
};

// Define the circular shift macro
#define nng_sha1_circular_shift(bits, word) \
    ((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32 - (bits))))

static void nng_sha1_process(nng_sha1_ctx *);
static void nng_sha1_pad(nng_sha1_ctx *);

// nng_sha1_init initializes the context to an initial value.
static void
nng_sha1_init(nng_sha1_ctx *ctx){
    ctx->len = 0;
    ctx->idx = 0;

    ctx->digest[0] = 0x67452301;
    ctx->digest[1] = 0xEFCDAB89;
    ctx->digest[2] = 0x98BADCFE;
    ctx->digest[3] = 0x10325476;
    ctx->digest[4] = 0xC3D2E1F0;
}

// nng_sha1_final runs the final padding for the digest, and stores
// the resulting digest in the supplied output buffer.
static void
nng_sha1_final(nng_sha1_ctx *ctx, uint8_t digest[20]){
    nng_sha1_pad(ctx);
    for (int i = 0; i < 5; i++) {
        digest[i * 4]     = (ctx->digest[i] >> 24) & 0xff;
        digest[i * 4 + 1] = (ctx->digest[i] >> 16) & 0xff;
        digest[i * 4 + 2] = (ctx->digest[i] >> 8) & 0xff;
        digest[i * 4 + 3] = (ctx->digest[i] >> 0) & 0xff;
    }
}

// nng_sha1 is a convenience that does the entire init, update, and final
// sequence in a single operation.
static void
nng_sha1(const void *msg, size_t length, uint8_t digest[20]){
    nng_sha1_ctx ctx;

    nng_sha1_init(&ctx);
    nng_sha1_update(&ctx, msg, length);
    nng_sha1_final(&ctx, digest);
}

// nng_sha1_update updates the SHA1 context, reading from the message supplied.
static void
nng_sha1_update(nng_sha1_ctx *ctx, const void *data, size_t length){
    const uint8_t *msg = data;

    if (!length) {
        return;
    }

    while (length--) {
        // memcpy might be faster...
        ctx->blk[ctx->idx++] = (*msg & 0xFF);
        ctx->len += 8;

        if (ctx->idx == 64) {
            // This will reset the index back to zero.
            nng_sha1_process(ctx);
        }

        msg++;
    }
}

// nng_sha1_process processes the next 512 bites of the message stored
// in the blk array.
static void
nng_sha1_process(nng_sha1_ctx *ctx){
    const unsigned K[] = // Constants defined in SHA-1
        { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
    unsigned temp;          // Temporary word value
    unsigned W[80];         // Word sequence
    unsigned A, B, C, D, E; // Word buffers

    // Initialize the first 16 words in the array W
    for (int t = 0; t < 16; t++) {
        W[t] = ((unsigned) ctx->blk[t * 4]) << 24;
        W[t] |= ((unsigned) ctx->blk[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) ctx->blk[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) ctx->blk[t * 4 + 3]);
    }

    for (int t = 16; t < 80; t++) {
        W[t] = nng_sha1_circular_shift(
            1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
    }

    A = ctx->digest[0];
    B = ctx->digest[1];
    C = ctx->digest[2];
    D = ctx->digest[3];
    E = ctx->digest[4];

    for (int t = 0; t < 20; t++) {
        temp = nng_sha1_circular_shift(5, A) + ((B & C) | ((~B) & D)) +
            E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = nng_sha1_circular_shift(30, B);
        B = A;
        A = temp;
    }

    for (int t = 20; t < 40; t++) {
        temp = nng_sha1_circular_shift(5, A) + (B ^ C ^ D) + E + W[t] +
            K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = nng_sha1_circular_shift(30, B);
        B = A;
        A = temp;
    }

    for (int t = 40; t < 60; t++) {
        temp = nng_sha1_circular_shift(5, A) +
            ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = nng_sha1_circular_shift(30, B);
        B = A;
        A = temp;
    }

    for (int t = 60; t < 80; t++) {
        temp = nng_sha1_circular_shift(5, A) + (B ^ C ^ D) + E + W[t] +
            K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = nng_sha1_circular_shift(30, B);
        B = A;
        A = temp;
    }

    ctx->digest[0] = (ctx->digest[0] + A) & 0xFFFFFFFF;
    ctx->digest[1] = (ctx->digest[1] + B) & 0xFFFFFFFF;
    ctx->digest[2] = (ctx->digest[2] + C) & 0xFFFFFFFF;
    ctx->digest[3] = (ctx->digest[3] + D) & 0xFFFFFFFF;
    ctx->digest[4] = (ctx->digest[4] + E) & 0xFFFFFFFF;

    ctx->idx = 0;
}

// nng_sha1_pad pads the message, adding the length.  This is done
// when finishing the message.
//
// According to the standard, the message must be padded to an even 512 bits.
// The first padding bit must be a '1'.  The last 64 bits represent the length
// of the original message.  All bits in between should be 0.  This function
// will pad the message according to those rules by filling the blk array
// accordingly. It will also call nng_sha1_process() appropriately.  When it
// returns, it can be assumed that the message digest has been computed.
static void
nng_sha1_pad(nng_sha1_ctx *ctx){
    // Check to see if the current message block is too small to hold
    // the initial padding bits and length.  If so, we will pad the
    // block, process it, and then continue padding into a second block.
    if (ctx->idx > 55) {
        ctx->blk[ctx->idx++] = 0x80;
        while (ctx->idx < 64) {
            ctx->blk[ctx->idx++] = 0;
        }

        nng_sha1_process(ctx);

        while (ctx->idx < 56) {
            ctx->blk[ctx->idx++] = 0;
        }
    } else {
        ctx->blk[ctx->idx++] = 0x80;
        while (ctx->idx < 56) {
            ctx->blk[ctx->idx++] = 0;
        }
    }

    // Store the message length as the last 8 octets (big endian)
    ctx->blk[56] = (ctx->len >> 56) & 0xff;
    ctx->blk[57] = (ctx->len >> 48) & 0xff;
    ctx->blk[58] = (ctx->len >> 40) & 0xff;
    ctx->blk[59] = (ctx->len >> 32) & 0xff;
    ctx->blk[60] = (ctx->len >> 24) & 0xff;
    ctx->blk[61] = (ctx->len >> 16) & 0xff;
    ctx->blk[62] = (ctx->len >> 8) & 0xff;
    ctx->blk[63] = (ctx->len) & 0xff;

    nng_sha1_process(ctx);
}

static void
sha1_lower_case(void *src, size_t src_siz, char res[41]){
    uint8_t r[20];
    nng_sha1(src, src_siz, r);

    src_siz = 0;
    for(; src_siz < 20; ++src_siz){
        sprintf(res + 2 * src_siz, "%02x", r[src_siz]);
    }
}

//@param filepath[in]:
//upper_case_or_not[in]: 0 for lowercase
//@param res[out]:
static Error *
sha1_file_lower_case(char *filepath, char res[41]){
    uint8_t r[20];
    nng_sha1_ctx ctx;

    char buf[8192];
    size_t len;

    _i fd, i;

    if(0 > (fd = open(filepath, O_RDONLY))){
        return __err_new(errno, strerror(errno), nil);
    }

    nng_sha1_init(&ctx);
    while(0 < (len = read(fd, buf, 8192))){
        nng_sha1_update(&ctx, buf, len);
    }

    nng_sha1_final(&ctx, r);

    for(i = 0; i < 20; ++i){
        sprintf(res + 2 * i, "%02x", r[i]);
    }

    return nil;
}
