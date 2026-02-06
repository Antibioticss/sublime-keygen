#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "private.h"
#include "lzcrypt/lzcrypt.h"

static const char hex_digits[] = "0123456789ABCDEF";

char *keygen(license_type_t type, const char *name, const char *id, int seats) {
    const char *type_name;
    if (type == SUBLIME_TEXT)
        type_name = "EA7E";
    else if (type == SUBLIME_MERGE)
        type_name = "E52D";
    else if (type == SUBLIME_BUNDLE)
        type_name = "E3D2";
    else {
        fprintf(stderr, "Unknown license type: %d\n", type);
        return NULL;
    }

    char id_tmp[20];
    if (id == NULL) {
        srand(time(NULL));
        snprintf(id_tmp, 20, "%08d", rand() % 100000000);
        id = id_tmp;
    }

    int hlen = strlen(name) + strlen(id) + 60;
    char *header = malloc(hlen);
    if (seats == 0)
        hlen = snprintf(header, hlen, "%s\nUnlimited User License\n%s-%s", name, type_name, id);
    else if (seats == 1)
        hlen = snprintf(header, hlen, "%s\nSingle User License\n%s-%s", name, type_name, id);
    else if (seats >= 2)
        hlen = snprintf(header, hlen, "%s\n%d User License\n%s-%s", name, seats, type_name, id);
    else {
        fprintf(stderr, "Invalid license seats: %d\n", seats);
        free(header);
        return NULL;
    }

    /* PKCS#1 v1.5 Signature */
    unsigned long slen = 128;
    unsigned char signature[128];
    z_rsassa_pkcs1v15_sign((unsigned char *)header, hlen, signature);

    /* construct license */
    int cur = 0;
    char *license_code = malloc(hlen + slen * 2 + 250);
    strcpy(license_code, "----- BEGIN LICENSE -----\n"); cur += 26;
    strcpy(license_code + cur, header);                  cur += hlen;
    license_code[cur++] = '\n';
    for (int i = 0; i < slen; i++) {
        license_code[cur++] = hex_digits[signature[i] >> 4];
        license_code[cur++] = hex_digits[signature[i] & 0xf];
        if ((i + 1) % 16 == 0) license_code[cur++] = '\n';
        else if ((i + 1) % 4 == 0) license_code[cur++] = ' ';
    }
    strcpy(license_code + cur, "------ END LICENSE ------\n");

    free(header);
    return license_code;
}
