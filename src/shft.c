/** @file
    shft: shift data files bitwise.

    Copyright (C) 2019 Christian W. Zuckschwerdt <zany@triq.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "codes.h"

static struct data data[LIST_MAX];
static unsigned msg_len  = 0;
static unsigned list_len = 0;

static void shift_bits(int bits)
{
    if (bits > 0) {
        // shift right
        int bytes = bits / 8;
        msg_len += bytes;
        bits = bits % 8;

        for (int i = 0; i < list_len; ++i) {
            memmove(&data[i].d[bytes], data[i].d, msg_len - bytes);
            memset(data[i].d, 0, bytes);
            for (int k = msg_len; k > bytes; --k) {
                data[i].d[k] >>= bits;
                data[i].d[k] |= data[i].d[k - 1] << (8 - bits);
            }
            data[i].d[bytes] >>= bits;
        }
    }
    else if (bits < 0) {
        // shift left
        int bytes = (-bits) / 8;
        msg_len -= bytes;
        bits = (-bits) % 8;

        for (int i = 0; i < list_len; ++i) {
            memmove(data[i].d, &data[i].d[bytes], msg_len);
            memset(&data[i].d[msg_len], 0, bytes);
            for (int k = 0; k < msg_len; ++k) {
                data[i].d[k] <<= bits;
                data[i].d[k] |= data[i].d[k + 1] >> (8 - bits);
            }
        }
    }
}

// trim bits off the right
static void trim_bits(int bits)
{
    if (bits > 0) {
        // trim right
        int bytes = bits / 8;
        msg_len -= bytes;
        bits = bits % 8;

        uint8_t mask = 0xff << bits;
        for (int i = 0; i < list_len; ++i) {
            data[i].d[msg_len - 1] &= mask;
        }
    }
    else if (bits < 0) {
        // pad right
        int bytes = (-bits + 7) / 8;
        msg_len += bytes;
    }
}

__attribute__((noreturn))
static void usage(int argc, char const *argv[])
{
    fprintf(stderr, "%s: [-s x] [-t x] codes.txt\n", argv[0]);
    exit(1);
}

int main(int argc, char const *argv[])
{
    unsigned verbose = 0;
    int shift_n = 0;
    int trim_n = 0;

    int i = 1;
    for (; i < argc; ++i) {
        if (*argv[i] != '-')
            break;
        if (argv[i][1] == 'h')
            usage(argc, argv);
        else if (argv[i][1] == 'v')
            verbose = 1;
        else if (argv[i][1] == 's')
            shift_n = atoi(argv[++i]);
        else if (argv[i][1] == 't')
            trim_n = atoi(argv[++i]);
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
    if (msg_len <= 0) {
        fprintf(stderr, "Message length too short!\n");
        usage(argc, argv);
    }

    if (verbose)
        fprintf(stderr, "Shifting all rows by %d bits, trimming %d bits...\n", shift_n, trim_n);

    shift_bits(shift_n);
    trim_bits(trim_n);

    printf("; codes shifted by %d, trimmed by %d\n", shift_n, trim_n);
    print_codes(data, msg_len, list_len);
}
