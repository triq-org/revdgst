/** @file
    revsum: reverse 8-bit (and 4-bit) simple checksums.

    Copyright (C) 2019 Christian W. Zuckschwerdt <zany@triq.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "intrinsic.h"
#include "util.h"
#include "measure.h"
#include "codes.h"

static struct data data[LIST_MAX];
static unsigned msg_len  = 0;
static unsigned list_len = 0;
static unsigned min_matches = 0;

/*
Modify these algorithms to test other possible checksum methods.

You can also add more algorithms (PR if you have a nice one!).
Remember to add any new algorithm to run_algos().

If you want to scan different parts of the messages change scan_algos().
*/

static void row_weight()
{
    unsigned max_weight = 0;
    unsigned min_weight = msg_len * 8;
    unsigned sum_weight = 0;

    for (int i = 0; i < list_len; ++i) {
        unsigned weight = 0;
        for (int j = 0; j < msg_len; ++j) {
            weight += popcount(data[i].d[j]);
        }
        if (weight > max_weight) max_weight = weight;
        if (weight < min_weight) min_weight = weight;
        sum_weight += weight;
    }
    double avg_weight = sum_weight / list_len;

    printf("Row weights:  ");
    printf("Max %u /%u bit (%.1f%%)  ", max_weight, msg_len * 8, max_weight * 100.0 / msg_len / 8);
    printf("Min %u /%u bit (%.1f%%)  ", min_weight, msg_len * 8, min_weight * 100.0 / msg_len / 8);
    printf("Avg %.1f /%u bit (%.1f%%)\n", avg_weight, msg_len * 8, avg_weight * 100.0 / msg_len / 8);
}

static void byte_sums(unsigned off, unsigned len, unsigned chk)
{
    int max_add = 0;
    int max_sub = 0;
    int max_xor = 0;
    uint8_t rem_add = 0;
    uint8_t rem_sub = 0;
    uint8_t rem_xor = 0;

    for (int i = 0; i < list_len; ++i) {
        int found_add = 0;
        int found_sub = 0;
        int found_xor = 0;
        uint8_t sumi = add_bytes(&data[i].d[off], len);
        uint8_t addi = sumi + data[i].d[chk];
        uint8_t subi = sumi - data[i].d[chk];
        uint8_t xori = sumi ^ data[i].d[chk];

        for (int j = 0; j < list_len; ++j) {
            uint8_t sumj = add_bytes(&data[j].d[off], len);
            uint8_t addj = sumj + data[j].d[chk];
            uint8_t subj = sumj - data[j].d[chk];
            uint8_t xorj = sumj ^ data[j].d[chk];

            if (addi == addj) found_add++;
            if (subi == subj) found_sub++;
            if (xori == xorj) found_xor++;
        }
        if (found_add > max_add) {
            max_add = found_add;
            rem_add = addi;
        }
        if (found_sub > max_sub) {
            max_sub = found_sub;
            rem_sub = subi;
        }
        if (found_xor > max_xor) {
            max_xor = found_xor;
            rem_xor = xori;
        }
    }

    if (max_add > min_matches) {
        printf("Found: add_bytes(&b[%u], %u) + b[%u] == 0x%02x; // (%.1f%%)\n", off, len, chk, rem_add, max_add * 100.0 / list_len);
    }
    if (max_sub > min_matches) {
        printf("Found: add_bytes(&b[%u], %u) - b[%u] == 0x%02x; // (%.1f%%)\n", off, len, chk, rem_sub, max_sub * 100.0 / list_len);
    }
    if (max_xor > min_matches) {
        printf("Found: add_bytes(&b[%u], %u) ^ b[%u] == 0x%02x; // (%.1f%%)\n", off, len, chk, rem_xor, max_xor * 100.0 / list_len);
    }
}

static void nibble_sums(unsigned off, unsigned len, unsigned chk)
{
    int max_add = 0;
    int max_sub = 0;
    int max_xor = 0;
    uint8_t rem_add = 0;
    uint8_t rem_sub = 0;
    uint8_t rem_xor = 0;

    for (int i = 0; i < list_len; ++i) {
        int found_add = 0;
        int found_sub = 0;
        int found_xor = 0;
        uint8_t sumi = add_nibbles(&data[i].d[off], len);
        uint8_t addi = sumi + data[i].d[chk];
        uint8_t subi = sumi - data[i].d[chk];
        uint8_t xori = sumi ^ data[i].d[chk];

        for (int j = 0; j < list_len; ++j) {
            uint8_t sumj = add_nibbles(&data[j].d[off], len);
            uint8_t addj = sumj + data[j].d[chk];
            uint8_t subj = sumj - data[j].d[chk];
            uint8_t xorj = sumj ^ data[j].d[chk];

            if (addi == addj) found_add++;
            if (subi == subj) found_sub++;
            if (xori == xorj) found_xor++;
        }
        if (found_add > max_add) {
            max_add = found_add;
            rem_add = addi;
        }
        if (found_sub > max_sub) {
            max_sub = found_sub;
            rem_sub = subi;
        }
        if (found_xor > max_xor) {
            max_xor = found_xor;
            rem_xor = xori;
        }
    }

    if (max_add > min_matches) {
        printf("Found: add_nibbles(&b[%u], %u) + b[%u] == 0x%02x; // (%.1f%%)\n", off, len, chk, rem_add, max_add * 100.0 / list_len);
    }
    if (max_sub > min_matches) {
        printf("Found: add_nibbles(&b[%u], %u) - b[%u] == 0x%02x; // (%.1f%%)\n", off, len, chk, rem_sub, max_sub * 100.0 / list_len);
    }
    if (max_xor > min_matches) {
        printf("Found: add_nibbles(&b[%u], %u) ^ b[%u] == 0x%02x; // (%.1f%%)\n", off, len, chk, rem_xor, max_xor * 100.0 / list_len);
    }
}

static void crc_scan(unsigned off, unsigned len, unsigned chk)
{
    for (int p = 1; p <= 255; ++p) {
        int found_max = 0;
        int found_poly = 0;
        int found_init = 0;

        for (int i = 0; i < list_len; ++i) {
            int found = 0;
            uint8_t chki = crc8(&data[i].d[off], len, p, 0x00) ^ data[i].d[chk];

            for (int j = 0; j < list_len; ++j) {
                uint8_t chkj = crc8(&data[j].d[off], len, p, 0x00) ^ data[j].d[chk];
                if (chki == chkj) {
                    found++;
                }
            }

            if (found > found_max) {
                found_max = found;
                found_poly = p;
                found_init = chki;
            }
        }

        if (found_max > min_matches) {
            printf("Found: crc8(&b[%u], %u, 0x%02x, 0x%02x) ^ b[%u] == 0; // (%.1f%%)\n",
                    off, len, found_poly, found_init, chk, found_max * 100.0 / list_len);
        }
    }
}


__attribute__((always_inline))
static inline uint8_t xor_shift_bytes(uint8_t const message[], unsigned num_bytes, int shifts)
{
    uint8_t result = 0;
    for(unsigned i = 0 ; i < num_bytes; ++i) {
        result ^= message[i] ;
        if (shifts > 0)
            result ^= (( result ^ ( result << shifts )) & 0x0F) << 4 ;
        else
            result ^= (( result ^ ( result >> (shifts*-1) )) & 0x0F) << 4 ;
    }
    return result;
}

static void xor_shift(unsigned off, unsigned len, unsigned chk)
{
    for (int shift = -4; shift <= 4; ++shift) {
        int found_max = 0;
        int found_shift = 0;

        for (int i = 0; i < list_len; ++i) {
            int found = 0;
            uint8_t chki = xor_shift_bytes(&data[i].d[off], len, shift) ^ data[i].d[chk];

            for (int j = 0; j < list_len; ++j) {
                uint8_t chkj = xor_shift_bytes(&data[j].d[off], len, shift) ^ data[j].d[chk];
                if (chki == chkj) {
                    found++;
                }
            }

            if (found > found_max) {
                found_max = found;
                found_shift = shift;
            }
        }

        if (found_max > min_matches) {
            printf("Found: xor_shift(&b[%u], %u, %d) ^ b[%u] == 0; // (%.1f%%)\n",
                    off, len, found_shift, chk, found_max * 100.0 / list_len);
        }
    }
}

static void run_algos(unsigned off, unsigned len, unsigned chk)
{
    byte_sums(off, len, chk);
    nibble_sums(off, len, chk);
    crc_scan(off, len, chk);
    xor_shift(off, len, chk);
}

static void scan_algos()
{
    fprintf(stderr, "Complete message...\n");
    run_algos(0, msg_len - 1, msg_len - 1);

    fprintf(stderr, "Skipping first byte...\n");
    run_algos(1, msg_len - 2, msg_len - 1);

    fprintf(stderr, "Omitting last byte...\n");
    run_algos(0, msg_len - 2, msg_len - 2);
}

__attribute__((noreturn))
static void usage(int argc, char const *argv[])
{
    fprintf(stderr, "%s: codes.txt\n", argv[0]);
    exit(1);
}

int main(int argc, char const *argv[])
{
    start_runtimes();

    int verbose = 0;
    double min_matches_pct = 0.5;

    int i = 1;
    for (; i < argc; ++i) {
        if (*argv[i] != '-')
            break;
        if (argv[i][1] == 'h')
            usage(argc, argv);
        else if (argv[i][1] == 'v')
            verbose++;
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

    min_matches = list_len * min_matches_pct;

    row_weight();

    fprintf(stderr, "Processing...\n");
    scan_algos();

    fprintf(stderr, "INVERT Inverting...\n");
    for (int j = 0; j < list_len; ++j) {
        invert_bytes(data[j].d, msg_len);
    }
    scan_algos();

    fprintf(stderr, "BYTE_REFLECT Reflecting...\n");
    for (int j = 0; j < list_len; ++j) {
        invert_bytes(data[j].d, msg_len);
        reflect_bytes(data[j].d, msg_len);
    }
    scan_algos();

    fprintf(stderr, "INVERT BYTE_REFLECT Inverting...\n");
    for (int j = 0; j < list_len; ++j) {
        invert_bytes(data[j].d, msg_len);
    }
    scan_algos();

    print_runtimes();
}
