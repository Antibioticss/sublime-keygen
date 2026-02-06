#ifndef SUBKG_PRIVATE_H
#define SUBKG_PRIVATE_H

typedef enum sublime_license_type {
    SUBLIME_TEXT,
    SUBLIME_MERGE,
    SUBLIME_BUNDLE
} license_type_t;


int patch(const char *path);

char *keygen(license_type_t type, const char *name, const char *id, int seats);

#endif