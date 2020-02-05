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

static void invert_bits(void)
{
    for (int i = 0; i < list_len; ++i) {
        struct data *d = &data[i];
        // flip whole bytes
        for (int k = 0; k < (d->bit_len / 8); ++k) {
            d->d[k] ^= 0xff;
        }
        // flip remainder
        d->d[(d->bit_len / 8)] ^= 0xff << (8 - d->bit_len % 8);
        // flip checksums
        d->chk ^= 0xff;
        d->chk16 ^= 0xffff;
    }
}

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

// find offset, limited to 64 bits for now
static int find_offset(struct data *d, struct data *f)
{
    if (f->bit_len > 64) {
        fprintf(stderr, "sync prefix too long (%u bits, max 64 bits)\n", f->bit_len);
        exit(1);
    }

    uint64_t test = 0;
    uint64_t patt = 0;
    uint64_t mask = (uint64_t)-1 << (64 - f->bit_len);
    // fill pattern from MSB down
    for (unsigned i = 0; i < (f->bit_len + 7) / 8; ++i)
        patt |= (uint64_t)f->d[i] << (7 - i) * 8;
    // fill test from MSB down
    for (unsigned i = 0; i < (d->bit_len + 7) / 8 && i < 8; ++i)
        test |= (uint64_t)d->d[i] << (7 - i) * 8;

    for (int pos = 0; pos < (int)d->bit_len - (int)f->bit_len; ++pos) {
        //printf("? %3d %016llx %016llx %016llx %016llx\n", pos, test, patt, mask, ((test ^ patt) & mask));
        if (((test ^ patt) & mask) == 0) {
            //printf("! %3d %016llx %016llx %016llx\n", pos, test, patt, mask);
            return pos;
        }
        test <<= 1;
        // TODO: refill bits now
    }
    return -1; // not found
}

static void sync_bits(struct data *f)
{
    if (!f || !f->bit_len)
        return;

    int m = 0;
    for (int j = 0; j < list_len; ++j) {
        int offs = find_offset(&data[j], f);
        if (offs < 0)
            continue; // not matching

        // shift left
        int bytes    = offs / 8;
        int msgj_len = (data[j].bit_len - offs + 7) / 8;
        int bits     = offs % 8;

        memmove(data[j].d, &data[j].d[bytes], msgj_len);
        memset(&data[j].d[msgj_len], 0, bytes);
        for (int k = 0; k < msgj_len; ++k) {
            data[j].d[k] <<= bits;
            data[j].d[k] |= data[j].d[k + 1] >> (8 - bits);
        }

        // move up
        memmove(&data[m], &data[j], sizeof(struct data));
        m++;
    }
    list_len = m;
}

__attribute__((noreturn))
static void usage(int argc, char const *argv[])
{
    fprintf(stderr, "%s: [-s x] [-t x] [-f x] [-i] codes.txt\n", argv[0]);
    exit(1);
}

int main(int argc, char const *argv[])
{
    unsigned verbose = 0;
    int invert = 0;
    int shift_n = 0;
    int trim_n = 0;
    struct data find_d = {0};

    int i = 1;
    for (; i < argc; ++i) {
        if (*argv[i] != '-')
            break;
        if (argv[i][1] == 'h')
            usage(argc, argv);
        else if (argv[i][1] == 'v')
            verbose++;
        else if (argv[i][1] == 'i')
            invert++;
        else if (argv[i][1] == 's')
            shift_n = atoi(argv[++i]);
        else if (argv[i][1] == 't')
            trim_n = atoi(argv[++i]);
        else if (argv[i][1] == 'f')
            parse_code(argv[++i], &find_d);
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

    if (invert & 1)
        invert_bits();

    sync_bits(&find_d);

    if (verbose)
        fprintf(stderr, "Shifting all rows by %d bits, trimming %d bits...\n", shift_n, trim_n);

    shift_bits(shift_n);
    trim_bits(trim_n);

    printf("; codes shifted by %d, trimmed by %d\n", shift_n, trim_n);
    print_codes(data, msg_len, list_len);
}
