/** @file
    bitbrk: break out bit changes and compare checksums.

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

#include "intrinsic.h"
#include "measure.h"
#include "codes.h"

static struct data data[LIST_MAX];
static unsigned msg_len  = 0;
static unsigned list_len = 0;

static void single_bits()
{
    char bufi[MSG_MAX * 3 + 1] = {0};
    char bufj[MSG_MAX * 3 + 1] = {0};
    printf("k : codei  chki  ->  codej  chkj  |  chki^chkj chki+chkj chki-chkj\n");

    for (int k = 0; k < msg_len * 8; ++k) {
        for (int i = 0; i < list_len; ++i) {
            uint8_t *codei = data[i].d;
            uint8_t chki   = data[i].chk;

            uint8_t codex[MSG_MAX];
            memcpy(codex, codei, msg_len);
            codex[k / 8] ^= 1 << (7 - k % 8); // MSB to LSB

            //uint8_t byte = 1 << (k & 7); // MOD 8
            //uint8_t nibble = 1 << (k & 3); // MOD 4

            for (int j = i + 1; j < list_len; ++j) {
                uint8_t *codej = data[j].d;
                uint8_t chkj     = data[j].chk;

                if (!memcmp(codex, codej, msg_len)) {
                    sprint_code(bufi, &data[i], msg_len);
                    sprint_code(bufj, &data[j], msg_len);
                    printf("%2d : %s %02x  ->  %s %02x  |  %02x %02x %02x\n",
                            k, bufi, chki, bufj, chkj,
                            chki ^ chkj, chki + chkj, chki - chkj);
                }
            }
        }
    }
}

static void n_bits(unsigned bits)
{
    char bufi[MSG_MAX * 3 + 1] = {0};
    char bufj[MSG_MAX * 3 + 1] = {0};
    char bufx[MSG_MAX * 3 + 1] = {0};
    printf("k : codei  chki  ->  codej  chkj  |  chki^chkj chki+chkj chki-chkj\n");

    for (int i = 0; i < list_len; ++i) {
        uint8_t *codei = data[i].d;
        uint8_t chki   = data[i].chk;

        for (int j = i + 1; j < list_len; ++j) {
            uint8_t *codej = data[j].d;
            uint8_t chkj   = data[j].chk;

            int pop = 0;
            struct data datax;
            for (unsigned k = 0; k < msg_len; ++k) {
                datax.d[k] = codei[k] ^ codej[k];
                pop += popcount(datax.d[k]);
            }

            if (pop == bits) {
                sprint_code(bufi, &data[i], msg_len);
                sprint_code(bufj, &data[j], msg_len);
                sprint_code(bufx, &datax, msg_len);
                printf("%s: %s %02x  ->  %s %02x  |  %02x %02x %02x\n",
                        bufx, bufi, chki, bufj, chkj,
                        chki ^ chkj, chki + chkj, chki - chkj);
            }
        }
    }
}

static void all_collisions()
{
    char bufi[MSG_MAX * 3 + 1] = {0};
    char bufj[MSG_MAX * 3 + 1] = {0};
    printf("codei  chki  ->  codej  chkj\n");

    for (int i = 0; i < list_len; ++i) {
        uint8_t chki = data[i].chk;

        for (int j = i + 1; j < list_len; ++j) {
            uint8_t chkj = data[j].chk;

            if (chki == chkj) {
                sprint_code(bufi, &data[i], msg_len);
                sprint_code(bufj, &data[j], msg_len);
                printf("%s %02x  ->  %s %02x\n",
                        bufi, chki, bufj, chkj);
            }
        }
    }
}

// locate a single bit change, augment keystream, repeat.
static void key_brk()
{
    unsigned bit_len = msg_len * 8;
    int *keystream;
    keystream = malloc(bit_len * sizeof(int));
    memset(keystream, -1, bit_len * sizeof(int));

    unsigned found;
    unsigned hits_tab[256];

    for (int round = 0; round < bit_len; ++round) {
        // find a single bit change
        found = 0;
        for (int k = 0; k < bit_len; ++k) {
            memset(hits_tab, 0, sizeof(hits_tab));
            unsigned hit_count = 0;

            for (int i = 0; i < list_len; ++i) {
                uint8_t *codei = data[i].d;
                uint8_t chki   = data[i].chk;

                uint8_t codex[MSG_MAX];
                memcpy(codex, codei, msg_len);
                codex[k / 8] ^= 1 << (7 - k % 8); // MSB to LSB

                for (int j = i + 1; j < list_len; ++j) {
                    uint8_t *codej = data[j].d;
                    uint8_t chkj   = data[j].chk;

                    if (!memcmp(codex, codej, msg_len)) {
                        // printf("%2d : %d(%02x)  ^  %d(%02x)  =  %02x\n",
                        //         k, i, chki, j, chkj, chki ^ chkj);
                        hits_tab[chki ^ chkj]++;
                        hit_count++;
                        found++;
                    }
                }

            }
            // show potential hits
            for (int i = 0; i < 256; ++i) {
                if (hits_tab[i]) {
                    double hit_pct = (double)hits_tab[i] / hit_count;
                    printf("; %2d : key %02x (%d/%d %.0f%%)\n",
                            k, i, hits_tab[i], hit_count, 100.0 * hit_pct);
                    if (hit_pct > 0.5)
                        keystream[k] = i;
                }
            }
        }
        if (!found)
            break;

        // apply keystream
        for (int i = 0; i < list_len; ++i) {
            for (int k = 0; k < bit_len; ++k) {
                uint8_t bit = 1 << (7 - (k % 8)); // msb to lsb
                if (keystream[k] >= 0 && data[i].d[k / 8] & bit) {
                    data[i].d[k / 8] ^= bit;
                    data[i].d[msg_len] ^= keystream[k];
                    data[i].chk ^= keystream[k];
                }
            }
        }
    }

    printf("; remaining data bits\n");
    print_codes(data, msg_len + 1, list_len);

    printf("; keystream for %d bits\n", bit_len);
    for (int k = 0; k < bit_len; ++k) {
        if (keystream[k] >= 0)
            printf("; 0x%02x, // key at bit %d\n", keystream[k], k);
        else
            printf("; 0, // key at bit %d not found\n", k);
    }

    free(keystream);
}

// e.g. bitbrk_codes_gtwt03.txt

__attribute__((noreturn))
static void usage(int argc, char const *argv[])
{
    fprintf(stderr, "%s: [-v] [-b n | -c | -k] codes.txt\n", argv[0]);
    exit(1);
}

int main(int argc, char const *argv[])
{
    start_runtimes();

    unsigned verbose  = 0;
    unsigned bits     = 0;
    unsigned collisions = 0;
    unsigned keybreak = 0;

    int i = 1;
    for (; i < argc; ++i) {
        if (*argv[i] != '-')
            break;
        if (argv[i][1] == 'h')
            usage(argc, argv);
        else if (argv[i][1] == 'v')
            verbose = 1;
        else if (argv[i][1] == 'b')
            bits = atoi(argv[++i]);
        else if (argv[i][1] == 'c')
            collisions = 1;
        else if (argv[i][1] == 'k')
            keybreak = 1;
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
        fprintf(stderr, "Processing...\n");
    msg_len -= 1; // use 8-bit chk

    if (collisions)
        all_collisions();
    else if (keybreak)
        key_brk();
    else if (bits > 0)
        n_bits(bits);
    else
        single_bits();

    print_runtimes();
}
