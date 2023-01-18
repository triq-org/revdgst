/** @file
    keylst: list key from LFSR generators.

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

/// Fibonacci LFSR-8. rev=0: shr, rev=1: shl
/// taps is the generator and should include the output bit (LSB/MSB)
static void lfsr_fib8_keys(int verbose, int rev, uint8_t taps, uint8_t init)
{
    uint8_t key = init;
    unsigned rounds = 0;
    do {
        if (verbose)
            fprintf(stderr, "Fibonacci key at round %3d : %02x\n", rounds, key);
        else
            fprintf(stderr, "%02x ", key);
        // e.g. (Fibonacci LFSR)
        /* taps: 16 14 13 11; feedback polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
        //uint8_t feedback  = ((key >> 0) ^ (key >> 2) ^ (key >> 3) ^ (key >> 5)) & 1;
        //key = (key >> 1) | (feedback << 7);
        if (rev) {
            if (parity(key & taps))
                key = (key << 1) | (1 << 0);
            else
                key = (key << 1);
        }
        else {
            if (parity(key & taps))
                key = (key >> 1) | (1 << 7);
            else
                key = (key >> 1);
        }
        ++rounds;
    } while (rounds < 256 && key && key != init);
    fprintf(stderr, "\n");
}

/// Galois LFSR-8. rev=0: shr, rev=1: shl,
/// gen are the taps and should include the output bit (LSB/MSB)
static void lfsr_gal8_keys(int verbose, int rev, uint8_t gen, uint8_t init)
{
    uint8_t key = init;
    unsigned rounds = 0;
    do {
        if (verbose)
            fprintf(stderr, "Galois key at round %3d : %02x\n", rounds, key);
        else
            fprintf(stderr, "%02x ", key);
        if (rev) {
            if (key & 0x80)
                key = (key << 1) ^ gen;
            else
                key = (key << 1);
        }
        else {
            if (key & 1)
                key = (key >> 1) ^ gen;
            else
                key = (key >> 1);
        }
        ++rounds;
    } while (rounds < 256 && key && key != init);
    fprintf(stderr, "\n");
}

/// Fibonacci LFSR-16. rev=0: shr, rev=1: shl
/// taps is the generator and should include the output bit (LSB/MSB)
static void lfsr_fib16_keys(int verbose, int rev, uint16_t taps, uint16_t init)
{
    uint16_t key = init;
    unsigned rounds = 0;
    do {
        if (verbose)
            fprintf(stderr, "Fibonacci key at round %3d : %04x\n", rounds, key);
        else
            fprintf(stderr, "%04x ", key);
        // e.g. (Fibonacci LFSR)
        /* taps: 16 14 13 11; feedback polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
        //uint16_t feedback  = ((key >> 0) ^ (key >> 2) ^ (key >> 3) ^ (key >> 5)) & 1;
        //key = (key >> 1) | (feedback << 7);
        if (rev) {
            if (parity(key & taps))
                key = (key << 1) | (1 << 0);
            else
                key = (key << 1);
        }
        else {
            if (parity(key & taps))
                key = (key >> 1) | (1 << 15);
            else
                key = (key >> 1);
        }
        ++rounds;
    } while (key && key != init);
    fprintf(stderr, "\n");
}

/// Galois LFSR-16. rev=0: shr, rev=1: shl,
/// gen are the taps and should include the output bit (LSB/MSB)
static void lfsr_gal16_keys(int verbose, int rev, uint16_t gen, uint16_t init)
{
    uint16_t key = init;
    unsigned rounds = 0;
    do {
        if (verbose)
            fprintf(stderr, "Galois key at round %3d : %04x\n", rounds, key);
        else
            fprintf(stderr, "%04x ", key);
        if (rev) {
            if (key & 0x80)
                key = (key << 1) ^ gen;
            else
                key = (key << 1);
        }
        else {
            if (key & 1)
                key = (key >> 1) ^ gen;
            else
                key = (key >> 1);
        }
        ++rounds;
    } while (key && key != init);
    fprintf(stderr, "\n");
}

int main(int argc, char const *argv[])
{
    unsigned verbose  = 0;
    unsigned reverse  = 0;
    unsigned width    = 8;
    unsigned lfsrtype = 1;
    unsigned gen      = 0;
    unsigned init     = 0;

    if (argc <= 1) {
        fprintf(stderr, "%s: [-v] -g xx -i xx [-G|-F] [-r] [-w 8|16]\n", argv[0]);
        return -1;
    }

    for (int i = 1; i < argc; ++i) {
        if (*argv[i] != '-') {
            fprintf(stderr, "Wrong arguments.\n");
            return -1;
        }
        if (argv[i][1] == 'v')
            verbose++;
        else if (argv[i][1] == 'r')
            reverse = 1;
        else if (argv[i][1] == 'G')
            lfsrtype = 1;
        else if (argv[i][1] == 'F')
            lfsrtype = 2;
        else if (argv[i][1] == 'g')
            sscanf(argv[++i], "%x", &gen);
        else if (argv[i][1] == 'i')
            sscanf(argv[++i], "%x", &init);
        else if (argv[i][1] == 'w')
            width = atoi(argv[++i]);
        else
            fprintf(stderr, "Wrong argument (%s).\n", argv[i]);
    }

    /*
    printf("0x0 : %d\n", parity(0x0));
    printf("0x1 : %d\n", parity(0x1));
    printf("0x2 : %d\n", parity(0x2));
    printf("0x3 : %d\n", parity(0x3));
    printf("0x4 : %d\n", parity(0x4));
    printf("0x1021 : %d\n", parity(0x1021));
    printf("0xff : %d\n", parity(0xff));
    exit(0);
    */

    if (width == 8 && lfsrtype == 1)
        lfsr_gal8_keys(verbose, reverse, gen, init);
    else if (width == 8 && lfsrtype == 2)
        lfsr_fib8_keys(verbose, reverse, gen, init);
    else if (width == 16 && lfsrtype == 1)
        lfsr_gal16_keys(verbose, reverse, gen, init);
    else if (width == 16 && lfsrtype == 2)
        lfsr_fib16_keys(verbose, reverse, gen, init);
}
