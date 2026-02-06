#ifndef SKIP_H
#define SKIP_H

/* skip -- a super fast string search algo */

#include <stddef.h>

typedef struct skipidx_ skipidx_t;

skipidx_t *skip_init(const unsigned char *pattern, size_t patlen);

int skip_match(skipidx_t *idx, const unsigned char *data, size_t dlen, size_t *outoff, int outlen);

void skip_release(skipidx_t *idx);

#endif
