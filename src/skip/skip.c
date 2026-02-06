#include <stdlib.h>
#include <string.h>

#include "skip.h"

/* alphabet size 0x0-0xff */
#define ASIZE 0x100

typedef struct {
    int val;
    int nxt;
} node_t;

struct skipidx_ {
    size_t         plen; /* pattern len */
    unsigned char *patt; /* original pattern */
    int           *buck; /* bucket storing indices */
    node_t        *buff; /* linked list buffer */
};

skipidx_t *skip_init(const unsigned char *pattern, size_t patlen) {
    skipidx_t     *idx    = malloc(sizeof(skipidx_t));
    unsigned char *patt   = malloc(patlen + 1);
    int           *bucket = malloc(ASIZE * sizeof(int));
    node_t        *buffer = malloc((patlen + 1) * sizeof(node_t));

    memcpy(patt, pattern, patlen);
    memset(bucket, 0, ASIZE * sizeof(int));

    int bufidx = 1;
    for (size_t i = 0; i < patlen; i++) {
        buffer[bufidx].val = i;
        buffer[bufidx].nxt = bucket[pattern[i]];
        bucket[pattern[i]] = bufidx;
        bufidx++;
    }

    idx->plen = patlen;
    idx->patt = patt;
    idx->buck = bucket;
    idx->buff = buffer;
    return idx;
}

int skip_match(skipidx_t *idx, const unsigned char *data, size_t dlen, size_t *outoff, int outlen) {
    size_t         patlen  = idx->plen;
    unsigned char *pattern = idx->patt;
    int           *bucket  = idx->buck;
    node_t        *buffer  = idx->buff;

    int matched = 0;

    size_t i;
    for (i = patlen - 1; i <= dlen - patlen - 1; i += patlen) {
        for (int j = bucket[data[i]]; j; j = buffer[j].nxt) {
            size_t cur = i - buffer[j].val;
            if (memcmp(pattern, data + cur, patlen) == 0) {
                outoff[matched++] = cur;
                if (matched == outlen) return matched;
            }
        }
    }
    for (int j = bucket[data[i]]; j; j = buffer[j].nxt) {
        size_t cur = i - buffer[j].val;
        if (cur + patlen <= dlen) {
            if (memcmp(pattern, data + cur, patlen) == 0) {
                outoff[matched++] = cur;
                if (matched == outlen) return matched;
            }
        }
    }

    return matched;
}

void skip_release(skipidx_t *idx) {
    free(idx->patt);
    free(idx->buck);
    free(idx->buff);
    memset(idx, 0, sizeof(skipidx_t));
    free(idx);
    return;
}
