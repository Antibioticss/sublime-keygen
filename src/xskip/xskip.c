#include <stdlib.h>
#include <string.h>

#include "xskip.h"

/* alphabet size 0x0-0xff */
#define ASIZE 0x100

typedef struct {
    int val;
    int nxt;
} node_t;

struct skipidx_ {
    size_t         plen; /* pattern len */
    unsigned char *patt; /* original pattern */
    int           *buck; /* bucket storing indices (xor-ed ch) */
    node_t        *buff; /* linked list buffer */
};

/*
b[i] = a[i] ^ K
b[i] ^ b[i+1] = a[i] ^ K ^ a[i+1] ^ K
              = a[i] ^ a[i+1]
*/

xskipidx_t *xskip_init(const unsigned char *pattern, size_t patlen) {
    /* patlen >= 2 */
    xskipidx_t    *idx    = malloc(sizeof(xskipidx_t));
    unsigned char *patt   = malloc(patlen);
    int           *bucket = malloc(ASIZE * sizeof(int));
    node_t        *buffer = malloc(patlen * sizeof(node_t));

    memcpy(patt, pattern, patlen);
    memset(bucket, 0, ASIZE * sizeof(int));

    int bufidx = 1;
    unsigned char *patx = malloc(patlen);
    for (size_t i = 0; i < patlen - 1; i++)
        patx[i] = patt[i] ^ patt[i + 1];
    for (size_t i = 0; i < patlen - 1; i++) {
        buffer[bufidx].val = i;
        buffer[bufidx].nxt = bucket[patx[i]];
        bucket[patx[i]] = bufidx;
        bufidx++;
    }
    free(patx);

    idx->plen = patlen;
    idx->patt = patt;
    idx->buck = bucket;
    idx->buff = buffer;
    return idx;
}

static int xor_cmp(const unsigned char *s1, const unsigned char *s2, size_t n) {
    unsigned char k = s1[0] ^ s2[0];
    for (size_t i = 0; i < n; i++) {
        if ((s1[i] ^ k) != s2[i])
            return 1;
    }
    return 0;
}

int xskip_match(xskipidx_t *idx, const unsigned char *data, size_t dlen, xoffset_t *outoff, int outlen) {
    size_t         patlen  = idx->plen;
    unsigned char *pattern = idx->patt;
    int           *bucket  = idx->buck;
    node_t        *buffer  = idx->buff;

    int matched = 0;
    const size_t steplen = patlen - 1;

    size_t i;
    unsigned char ch;
    for (i = steplen - 1; i <= dlen - steplen - 2; i += steplen) {
        ch = data[i] ^ data[i + 1];
        for (int j = bucket[ch]; j; j = buffer[j].nxt) {
            size_t cur = i - buffer[j].val;
            if (xor_cmp(pattern, data + cur, patlen) == 0) {
                outoff[matched].off = cur;
                outoff[matched].msk = pattern[0] ^ data[cur];
                if (++matched == outlen) return matched;
            }
        }
    }
    ch = data[i] ^ data[i + 1];
    for (int j = bucket[ch]; j; j = buffer[j].nxt) {
        size_t cur = i - buffer[j].val;
        if (cur + patlen <= dlen) { /* add boundary check in the last round */
            if (xor_cmp(pattern, data + cur, patlen) == 0) {
                outoff[matched].off = cur;
                outoff[matched].msk = pattern[0] ^ data[cur];
                if (++matched == outlen) return matched;
            }
        }
    }

    return matched;
}

void xskip_release(xskipidx_t *idx) {
    free(idx->patt);
    free(idx->buck);
    free(idx->buff);
    memset(idx, 0, sizeof(xskipidx_t));
    free(idx);
    return;
}
