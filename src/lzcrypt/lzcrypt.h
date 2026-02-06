#ifndef LZCRYPT_H
#define LZCRYPT_H

/* Light Zig crypto lib, slower but works :3 */

#include <stdlib.h>

/* RSASSA-PKCS#1-v1.5 + SHA1 (RSA-1024), signature is fixed 128 bytes (1024bit) */
int z_rsassa_pkcs1v15_sign(unsigned char *in, size_t inlen, unsigned char *signature);

#endif
