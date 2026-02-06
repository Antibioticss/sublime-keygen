#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "private.h"

static void usage() {
    puts("subkg - sublime products keygen v1.0");
    puts("Usage: subkg <command> [options]");
    puts("Commands:");
    puts("  patch <path>");
    puts("    (no option needed)");
    puts("  keygen [options]");
    puts("    -n <name>      name for the license");
    puts("    -t <type>      can be: bundle (default), text, merge");
    puts("    -s <seats>     license seats, pass 0 for unlimited seats (default)");
    puts("    -i <id>        license id (number)");
    puts("Made with love by Antibiotics <3");
    return;
}

static license_type_t  o_type  = SUBLIME_BUNDLE;
static const char     *o_name  = "Antibiotics";
static const char     *o_id    = NULL;
static long int        o_seats = 0;

static int parse_arg(int argc, char **argv) {
    int cur = 0;
    while (cur < argc) {
        char *arg = argv[cur++];
        char *end = NULL; /* used in strtol */
        if (strlen(arg) != 2 || arg[0] != '-') {
            fprintf(stderr, "Invalid option: '%s'\n", arg);
            return -1;
        }
        if (cur == argc) {
            if (arg[1] == 'n' || arg[1] == 't' || arg[1] == 's' || arg[1] == 'i')
                fprintf(stderr, "Option '%s' requires an argument\n", arg);
            else
                fprintf(stderr, "Unknown option: '%s'\n", arg);
            return -1;
        }
        switch (arg[1]) {
        case 'n':
            o_name = argv[cur++];
            break;
        case 't':
            arg = argv[cur++];
            if (strcmp(arg, "text") == 0)
                o_type = SUBLIME_TEXT;
            else if (strcmp(arg, "merge") == 0)
                o_type = SUBLIME_MERGE;
            else if (strcmp(arg, "bundle") == 0)
                o_type = SUBLIME_BUNDLE;
            else {
                fprintf(stderr, "Invalid license type: '%s'\n", arg);
                return -1;
            }
            break;
        case 's':
            arg = argv[cur++];
            o_seats = strtol(arg, &end, 10);
            if (*end != '\0' || o_seats >= INT_MAX || o_seats < 0) {
                fprintf(stderr, "Invalid seats: '%s'\n", arg);
                return -1;
            }
            break;
        case 'i':
            arg = argv[cur++];
            for (char *c = arg; *c; c++) {
                if (*c < '0' || *c > '9') {
                    fprintf(stderr, "Invalid license id: '%s'\n", arg);
                    return -1;
                }
            }
            o_id = arg;
            break;
        default:
            fprintf(stderr, "Unknown option: '%s'\n", arg);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "-h") == 0)) {
        usage();
        return 0;
    }

    if (strcmp(argv[1], "patch") == 0) {
        if (argc < 3) {
            fprintf(stderr, "'patch' command  requires a <path> argument\n");
            return -1;
        }
        if (argc > 3) {
            fprintf(stderr, "'patch' command only needs one argument\n");
            return -1;
        }
        if (patch(argv[2]) != 0)
            return -1;
    }
    else if (strcmp(argv[1], "keygen") == 0) {
        if (parse_arg(argc - 2, argv + 2) != 0) {
            return -1;
        }
        char *license_code = keygen(o_type, o_name, o_id, (int)o_seats);
        if (license_code == NULL)
            return -1;
        if (o_type == SUBLIME_TEXT)
            printf("Your license for Sublime Text:\n");
        else if (o_type == SUBLIME_MERGE)
            printf("Your license for Sublime Merge:\n");
        else // (o_type == SUBLIME_BUNDLE)
            printf("Your license for Sublime Text and Sublime Merge:\n");
        printf("%s", license_code);
        free(license_code);
    }
    else {
        fprintf(stderr, "Unknown command '%s'\n", argv[1]);
        return -1;
    }
    return 0;
}