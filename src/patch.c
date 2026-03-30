#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "private.h"
#include "skip/skip.h"
#include "xskip/xskip.h"

/* xor search + patch on file */

#define CHUNK_SIZE    1024*1024

typedef unsigned char u8;

typedef struct {
    unsigned char *pattern;
    unsigned char *replace;
    size_t         patlen;
    size_t        *outoff;
    int            outlen;
    int            matched;
} patch_t;

/* global regular patches table */
static int g_pcount;
static patch_t g_patches[20];

/* only public key needs xor-skip search */
static int pubkey_need = 2;
static int pubkey_matched;
static xoffset_t pubkey_off[2];

static const size_t pubkey_len = 160;
static const unsigned char sublime_pubkey[] = {
  0x30,0x81,0x9d,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x01,0x05,0x00,0x03,0x81,
  0x8b,0x00,0x30,0x81,0x87,0x02,0x81,0x81,0x00,0xd8,0x7b,0xa2,0x45,0x62,0xf7,0xc5,0xd1,0x4a,0x0c,0xfb,
  0x12,0xb9,0x74,0x0c,0x19,0x5c,0x6b,0xdc,0x7e,0x6d,0x6e,0xc9,0x2b,0xac,0x0e,0xb2,0x9d,0x59,0xe1,0xd9,
  0xae,0x67,0x89,0x0c,0x2b,0x88,0xc3,0xab,0xdc,0xaf,0xfe,0x7d,0x4a,0x33,0xdc,0xc1,0xbf,0xbe,0x53,0x1a,
  0x25,0x1c,0xef,0x0c,0x92,0x3f,0x06,0xbe,0x79,0xb2,0x32,0x85,0x59,0xac,0xfe,0xe9,0x86,0xd5,0xe1,0x5e,
  0x4d,0x17,0x66,0xea,0x56,0xc4,0xe1,0x06,0x57,0xfa,0x74,0xdb,0x09,0x77,0xc3,0xfb,0x75,0x82,0xb7,0x8c,
  0xd4,0x7b,0xb2,0xc7,0xf9,0xb2,0x52,0xb4,0xa9,0x46,0x3d,0x15,0xf6,0xae,0x6e,0xe9,0x23,0x7d,0x54,0xc5,
  0x48,0x1b,0xf3,0xe0,0xb0,0x99,0x20,0x19,0x0b,0xcf,0xb3,0x1e,0x5b,0xe5,0x09,0xc3,0x3b,0x02,0x01,0x11
};
static const unsigned char my_pubkey[] = {
  0x30,0x81,0x9d,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x01,0x05,0x00,0x03,0x81,
  0x8b,0x00,0x30,0x81,0x87,0x02,0x81,0x81,0x00,0xcd,0xaa,0x77,0x25,0x4e,0xe5,0x06,0x1a,0x20,0x4f,0x29,
  0x7c,0xae,0x36,0xcb,0xd5,0xfb,0xdb,0xcd,0x17,0x6e,0xec,0x71,0x71,0x6b,0xc6,0x18,0x1e,0x5b,0xff,0x55,
  0xeb,0x0f,0xe2,0x6e,0x30,0xcb,0xac,0x4c,0xc7,0xbc,0x48,0xc9,0xb8,0x0c,0xde,0xcb,0x70,0x17,0xd7,0x31,
  0xd8,0x20,0xda,0x83,0x75,0x48,0x57,0x9c,0x67,0x42,0x78,0x4b,0x74,0x14,0xfb,0xa0,0x8e,0xa3,0x78,0xad,
  0x8f,0x26,0xd7,0x47,0x59,0xe4,0xcd,0xd9,0xdb,0x77,0x37,0x27,0x96,0x4c,0x7d,0x35,0xd4,0x67,0xd0,0xae,
  0x22,0x43,0xac,0x8f,0xd5,0xf4,0x29,0x68,0xba,0x1e,0x40,0xdd,0xb8,0xf5,0xe0,0x05,0x58,0xa2,0x78,0xf3,
  0x95,0x3d,0xaa,0x72,0x9c,0xb9,0xeb,0x03,0xda,0x8a,0xe2,0x4f,0x33,0xe8,0x9b,0x3d,0x9f,0x02,0x01,0x11
};

static void add_patch(const unsigned char *pattern, const unsigned char *replace, size_t patlen, int expected) {
    patch_t *cp = g_patches + g_pcount;
    int outlen  = expected + 1;
    cp->pattern = malloc(patlen);
    cp->replace = malloc(patlen);
    cp->outoff  = malloc(outlen * sizeof(size_t));
    memcpy(cp->pattern, pattern, patlen);
    memcpy(cp->replace, replace, patlen);
    cp->patlen  = patlen;
    cp->outlen  = outlen;
    cp->matched = 0;
    g_pcount++;
}

static void free_patches() {
    for (int i = 0; i < g_pcount; i++) {
        free(g_patches[i].pattern);
        free(g_patches[i].replace);
        free(g_patches[i].outoff);
    }
    memset(g_patches, 0, g_pcount * sizeof(patch_t));
    g_pcount = 0;
}

static int chunked_search(FILE *fp) {
    int max_plen = pubkey_len;

    /* init public key xskipidx first */
    xskipidx_t *xskipidx = xskip_init(sublime_pubkey, pubkey_len);
    /* then skipidx */
    skipidx_t **skipidxs = malloc(g_pcount * sizeof(void *));
    for (int i = 0; i < g_pcount; i++) {
        skipidxs[i] = skip_init(g_patches[i].pattern, g_patches[i].patlen);
        if (g_patches[i].patlen > max_plen) max_plen = g_patches[i].patlen; /* and update max plen */
    }

    size_t rlen;
    size_t filepos = ftell(fp);
    unsigned char *buffer = malloc(CHUNK_SIZE + max_plen - 1);

    rlen = fread(buffer + max_plen - 1, 1, CHUNK_SIZE, fp);
    /* public key */
    pubkey_matched = xskip_match(xskipidx, buffer + max_plen - 1, rlen, pubkey_off, pubkey_need);
    for (int i = 0; i < pubkey_matched; i++) pubkey_off[i].off += filepos;
    /* other patches */
    for (int i = 0; i < g_pcount; i++) {
        patch_t *cp = g_patches + i;
        cp->matched = skip_match(skipidxs[i], buffer + max_plen - 1, rlen, cp->outoff, cp->outlen);
        for (int j = 0; j < cp->outlen; j++) cp->outoff[j] += filepos;
    }
    filepos += rlen;

    while (!feof(fp)) {
        memmove(buffer, buffer + CHUNK_SIZE, max_plen - 1);
        rlen = fread(buffer + max_plen - 1, 1, CHUNK_SIZE, fp);
        if (rlen == 0) {
            if (ferror(fp)) perror("fread");
            break;
        }
        /* public key */
        if (pubkey_matched != pubkey_need) {
            int cur = xskip_match(xskipidx, buffer + max_plen - pubkey_len, rlen + pubkey_len - 1, pubkey_off + pubkey_matched, pubkey_need - pubkey_matched);
            for (int i = 0; i < cur; i++) pubkey_off[pubkey_matched + i].off += filepos - (pubkey_len - 1);
            pubkey_matched += cur;
        }
        /* other patches */
        for (int i = 0; i < g_pcount; i++) {
            patch_t *cp = g_patches + i;
            if (cp->matched == cp->outlen) continue;
            int cur = skip_match(skipidxs[i], buffer + max_plen - cp->patlen, rlen + cp->patlen - 1, cp->outoff + cp->matched, cp->outlen - cp->matched);
            for (int j = 0; j < cur; j++) cp->outoff[cp->matched + j] += filepos - (cp->patlen - 1);
            cp->matched += cur;
        }
        filepos += rlen;
    }

    free(buffer);
    xskip_release(xskipidx);
    for (int i = 0; i < g_pcount; i++)
        skip_release(skipidxs[i]);
    free(skipidxs);

    return 0;
}

static int xor_patch(FILE *fp, const unsigned char *patch, size_t plen, xoffset_t *offsets, int offlen) {
    int patched = 0;
    unsigned char *buffer = malloc(plen);

    for (int i = 0; i < offlen; i++) {
        for (int j = 0; j < plen; j++)
            buffer[j] = patch[j] ^ offsets[i].msk;
        fseek(fp, offsets[i].off, SEEK_SET);
        if (fwrite(buffer, 1, plen, fp) != plen) {
            perror("fwrite");
            break;
        }
        patched++;
    }

    free(buffer);
    return patched;
}

static int apply_patch(FILE *fp, const unsigned char *patch, size_t plen, size_t *offsets, int offlen) {
    int patched = 0;

    for (int i = 0; i < offlen; i++) {
        fseek(fp, offsets[i], SEEK_SET);
        if (fwrite(patch, 1, plen, fp) != plen) {
            perror("fwrite");
            break;
        }
        patched++;
    }

    return patched;
}

#define ARCH_ARM64      1
#define ARCH_X86_64     2

int read_arch(FILE *fp) {
    unsigned char magic[16]; // e_ident
    if (fread(magic, 1, 16, fp) != 16) {
        fprintf(stderr, "File too small!\n");
        return -1;
    }

    int arches = 0;

    if (*(uint32_t *)magic == 0xfeedfacf || // MH_MAGIC_64
        *(uint32_t *)magic == 0xbebafeca) { // FAT_CIGAM
        /* macOS Mach-O file */
        arches = ARCH_ARM64 | ARCH_X86_64;
    }
    else if (memcmp(magic, "\x7f""ELF", 4) == 0) {
        /* Linux ELF file */
        uint16_t elf_header[2]; // e_type and e_machine
        if (fread(elf_header, 2, 2, fp) != 2) {
            fprintf(stderr, "File too small!\n");
            return -1;
        }
        if (elf_header[0] != 2 && elf_header[0] != 3) { // ET_EXEC / ET_DYN
            fprintf(stderr, "Not an executable/PIE file!\n");
            return -1;
        }
        if (elf_header[1] == 183) // EM_AARCH64
            arches = ARCH_ARM64;
        else if (elf_header[1] == 62) // EM_X86_64
            arches = ARCH_X86_64;
        else {
            fprintf(stderr, "Unknown arch %d for ELF file!\n", elf_header[1]);
            return -1;
        }
    }
    else if (memcmp(magic, "MZ", 2) == 0) {
        /* Windows PE file */
        arches = ARCH_X86_64;
    }
    else {
        fprintf(stderr, "Not an executable file!\n");
        return -1;
    }
    return arches;
}

int patch(const char *path) {
    int err = 0;

    FILE *fp = fopen(path, "rb+");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }

    int arches = read_arch(fp);
    if (arches < 0) {
        err = -1;
        goto exit;
    }

    /* extra patches list */
    /* block online verification */
    add_patch((u8 *)"license.sublimehq.com", (u8 *)"127.0.0.1/lol.atb.com", 21, 2);

    if (arches & ARCH_ARM64) {
        /* macOS arm64 patches */
        /* for merge 2121 */
        // cmp    w8, #0xc6 ; -> 0x82
        add_patch((u8[]){0x1f,0x19,0x03,0x71}, (u8[]){0x1f,0x09,0x02,0x71}, 4, 1);
        // mov    w9, #0xa5 ; -> 0xda
        add_patch((u8[]){0xa9,0x14,0x80,0x52}, (u8[]){0x49,0x1b,0x80,0x52}, 4, 1);
        /* extra one for text 4200 */
        // mov    w9, #0xc6 ; -> 0x82
        add_patch((u8[]){0xc9,0x18,0x80,0x52}, (u8[]){0x49,0x10,0x80,0x52}, 4, 1);
        /* extra two for text 4199 */
        // cmp    w8, #0x34 ; -> 0x03
        add_patch((u8[]){0x1f,0xd1,0x00,0x71}, (u8[]){0x1f,0x0d,0x00,0x71}, 4, 1);
        // mov    w9, #0xce ; -> 0x74
        add_patch((u8[]){0xc9,0x19,0x80,0x52}, (u8[]){0x89,0x0e,0x80,0x52}, 4, 1);
        /* no extra needed for merge 2120 */

        /* Linux arm64 patches */
        /* for merge 2121 */
        // mov    w10, #0xa5 ; -> 0xda
        add_patch((u8[]){0xaa,0x14,0x80,0x52}, (u8[]){0x4a,0x1b,0x80,0x52}, 4, 1);
        /* for text 4199 */
        // cmp    w8, #0xa5 ; -> 0xda
        add_patch((u8[]){0x1f,0x95,0x02,0x71}, (u8[]){0x1f,0x69,0x03,0x71}, 4, 1);
        /* no extra needed for text 4200 and merge 2120 */
    }
    if (arches & ARCH_X86_64) {
        /* macOS x86_64 patches */
        /* for merge 2121 and text 4200 */
        // xor    al, 0xc6 ; -> 0x82
        add_patch((u8[]){0x34,0xc6,0x80,0xf1}, (u8[]){0x34,0x82,0x80,0xf1}, 4, 1);
        // xor    cl, 0xa5 ; -> 0xda
        add_patch((u8[]){0x80,0xf1,0xa5}, (u8[]){0x80,0xf1,0xda}, 3, 1); // <- ?
        /* for text 4199 */
        // xor    al, 0x34 ; -> 0x03
        add_patch((u8[]){0x34,0x34,0x80,0xf1}, (u8[]){0x34,0x03,0x80,0xf1}, 4, 1);
        // xor    cl, 0xce ; -> 0x74
        add_patch((u8[]){0x80,0xf1,0xce}, (u8[]){0x80,0xf1,0x74}, 3, 1);

        /* Linux x86_64 patches */
        /* for text 4199 */
        // xor    al, 0xa5 ; -> 0xda
        add_patch((u8[]){0x34,0xa5,0x80,0xf1}, (u8[]){0x34,0xda,0x80,0xf1}, 4, 1);
        /* for merge 2121 */
        // xor    cl, 0xd7 ; -> 0x1f
        add_patch((u8[]){0x80,0xf1,0xd7}, (u8[]){0x80,0xf1,0x1f}, 3, 1);
        /* no extra needed for text 4200 and merge 2120 */

        /* Windows patches */
        /* for merge 2123 */
        // xor    al, 0xd7 ; -> 0x1f
        add_patch((u8[]){0x34,0xd7,0x41,0x8A}, (u8[]){0x34,0x1f,0x41,0x8A}, 4, 1);
        // xor    cl, 0xa8 ; -> 0x06
        add_patch((u8[]){0x80,0xf1,0xa8}, (u8[]){0x80,0xf1,0x06}, 3, 1);
        /* no extra needed for text 4200, merge 2121, text 4199 and merge 2120 */
    }

    if (chunked_search(fp) != 0) {
        err = -1;
        goto exit;
    }

    /* public key */
    if (pubkey_matched == 0) {
        fprintf(stderr, "Public key not found!\n");
        err = -1;
        goto exit;
    }
    int patched = xor_patch(fp, my_pubkey, pubkey_len, pubkey_off, pubkey_matched);
    if (patched != pubkey_matched) {
        fprintf(stderr, "Error patching public key, %d/%d match patched\n", patched, pubkey_matched);
        err = -1;
        goto exit;
    }
    printf("%d public key patched\n", patched);

    /* other patches */
    patched = 0;
    for (int i = 0; i < g_pcount; i++) {
        patch_t *cp = g_patches + i;
        if (cp->matched == 0) continue;
        if (cp->matched == cp->outlen) {
            fprintf(stderr, "Found more matches than expected for patch %d, skipping..\n", i);
            continue;
        }
        if (apply_patch(fp, cp->replace, cp->patlen, cp->outoff, cp->matched) != cp->matched) {
            fprintf(stderr, "Error applying extra patches, %d/%d patch applied\n", patched, g_pcount);
            err = -1;
            goto exit;
        }
        patched++;
    }
    printf("%d extra patches applied\n", patched);

exit:
    free_patches();
    fclose(fp);
    return err;
}
