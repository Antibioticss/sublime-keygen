#ifndef XSKIP_H
#define XSKIP_H

/* extended skip search algo for xor-ed strings */

#include <stddef.h>

typedef struct {
    unsigned char msk;
    size_t        off;
} xoffset_t;
typedef struct skipidx_ xskipidx_t;

xskipidx_t *xskip_init(const unsigned char *pattern, size_t patlen);

int xskip_match(xskipidx_t *idx, const unsigned char *data, size_t dlen, xoffset_t *outoff, int outlen);

void xskip_release(xskipidx_t *idx);

#endif
