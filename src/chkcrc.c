/** @file
    chkcrc: check for invalid crcs.

    Copyright (C) 2020 Christian W. Zuckschwerdt <zany@triq.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "util.h"
#include "measure.h"
#include "codes.h"

static struct data data[LIST_MAX];
static unsigned msg_len  = 0;
static unsigned list_len = 0;

__attribute__((noreturn))
static void usage(int argc, char const *argv[])
{
    fprintf(stderr, "%s: -p POLY [-i INIT] [-x FINAL-XOR] codes.txt\n", argv[0]);
    exit(1);
}

int main(int argc, char const *argv[])
{
    start_runtimes();

    int verbose = 0;
    int poly = 0;
    int init = 0;
    int fxor = 0;

    int i = 1;
    for (; i < argc; ++i) {
        if (*argv[i] != '-')
            break;
        if (argv[i][1] == 'h')
            usage(argc, argv);
        else if (argv[i][1] == 'v')
            verbose++;
        else if (argv[i][1] == 'p')
            poly = (int)strtol(argv[++i], NULL, 16);
        else if (argv[i][1] == 'i')
            init = (int)strtol(argv[++i], NULL, 16);
        else if (argv[i][1] == 'x')
            fxor = (int)strtol(argv[++i], NULL, 16);
        else {
            fprintf(stderr, "Wrong argument (%s).\n", argv[i]);
            usage(argc, argv);
        }
    }

    if (argc <= i) {
        fprintf(stderr, "Reading STDIN...\n");
    }
    list_len = read_codes(argv[i], data, &msg_len, MSG_MAX, LIST_MAX);
    if (list_len <= 0) {
        fprintf(stderr, "Missing data!\n");
        usage(argc, argv);
    }
    if (msg_len <= 1) {
        fprintf(stderr, "Message length too short!\n");
        usage(argc, argv);
    }

    if (verbose)
        print_codes(data, msg_len, list_len);

    for (int j = 0; j < list_len; ++j) {
        struct data *d = &data[j];
        int chk;
        if (poly > 255)
            chk = crc16(d->d, msg_len, poly, init);
        else
            chk = crc8(d->d, msg_len, poly, init);

        if (chk != fxor) {
            if (d->comment)
                free(d->comment);
            d->comment = strdup("; BAD CRC\n");
        }
    }

    print_runtimes();

    print_codes(data, msg_len, list_len);
}
