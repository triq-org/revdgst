/** @file
    revdgst: reverse 8-bit LFSR digest.

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

#include "job.h"

/*
Modify these algorithms to test other possible checksum methods.

You can also add more algorithms (PR if you have a nice one!).
Remember to add any new algorithm to call_algo() and then job_run() and increase it's job count return value.

Note that the reversing arguments will be expanded and optimized away when unrolling every code path in job_run().
There is no overhead in making an algorithm consume data bits in multiple possible ways.
*/

__attribute__((always_inline))
static inline void algo_fletcher8(int y_rev, int i_rev, int rev,
        uint8_t const *msg, int bytes, uint8_t gen, uint8_t key, uint8_t *sum_add, uint8_t *sum_xor)
{
    // all this will be optimized away
    int y_min, y_max, y_step;
    if (y_rev) {
        y_min = bytes - 1; y_max = -1; y_step = -1;
    } else {
        y_min = 0; y_max = bytes; y_step = 1;
    }

    uint8_t c0 = key & 0xf;
    uint8_t c1 = key >> 4;
    for (int k = y_min; k != y_max; k += y_step) {
        u_int8_t data = msg[k];
        if (i_rev) {
            data = reflect8(data);
        }

        c0 = (c0 + (data >> 4)) % 15;
        c1 = (c1 + c0) % 15;

        c0 = (c0 + (data & 0xf)) % 15;
        c1 = (c1 + c0) % 15;
    }
    if (rev) {
        *sum_add = c0 << 4 | c1;
    }
    else {
        *sum_add = c1 << 4 | c0;
    }
    *sum_xor = *sum_add; // bogus
}

__attribute__((always_inline))
static inline void algo_shift16_8(int y_rev, int i_rev, int rev,
        uint8_t const *msg, int bytes, uint8_t key1, uint8_t key2, uint8_t *sum_add, uint8_t *sum_xor)
{
    // all this will be optimized away
    int y_min, y_max, y_step;
    if (y_rev) {
        y_min = bytes - 1; y_max = -1; y_step = -1;
    } else {
        y_min = 0; y_max = bytes; y_step = 1;
    }

    int i_min, i_max, i_step;
    if (i_rev) {
        i_min = 7; i_max = -1; i_step = -1;
    }
    else {
        i_min = 0; i_max = 8; i_step = 1;
    }

    unsigned sum = 0;
    uint8_t xor = 0;
    uint16_t key = (key1 << 8) | key2;
    for (int k = y_min; k != y_max; k += y_step) {
        u_int8_t data = msg[k];
        for (int bit = i_min; bit != i_max; bit += i_step) {
            if ((data >> bit) & 1) {
                sum += key & 0xff;
                xor ^= key & 0xff;
            }

            if (rev) {
                if (key & 0x8000)
                    key = (key << 1) ^ 0x0001;
                else
                    key = (key << 1);
            }
            else {
                if (key & 1)
                    key = (key >> 1) ^ 0x8000;
                else
                    key = (key >> 1);
            }
        }
    }
    *sum_add = sum;
    //*sum_add = sum | (sum >> 8);
    *sum_xor = xor;
}

__attribute__((always_inline))
static inline void algo_elcheapo8(int y_rev, int i_rev, int rev,
        uint8_t const *msg, int bytes, uint8_t gen, uint8_t key, uint8_t *sum_add, uint8_t *sum_xor)
{
    // all this will be optimized away
    int y_min, y_max, y_step;
    if (y_rev) {
        y_min = bytes - 1; y_max = -1; y_step = -1;
    } else {
        y_min = 0; y_max = bytes; y_step = 1;
    }

    unsigned sum = key;
    uint8_t xor = key;
    for (int k = y_min; k != y_max; k += y_step) {
        u_int8_t data = msg[k];
        xor <<= 1;
        for (int bit = 0; bit < 8; ++bit) {
            if ((gen >> bit) & 1) {
                sum += data << bit;
                xor ^= data << bit;
            }
        }
    }
    *sum_add = sum;
    //*sum_add = sum | (sum >> 8);
    *sum_xor = xor;
}

__attribute__((always_inline))
static inline void algo_crc8(int y_rev, int i_rev, int rev,
        uint8_t const *msg, int bytes, uint8_t gen, uint8_t key, uint8_t *sum_add, uint8_t *sum_xor)
{
    // all this will be optimized away
    int y_min, y_max, y_step;
    if (y_rev) {
        y_min = bytes - 1; y_max = -1; y_step = -1;
    } else {
        y_min = 0; y_max = bytes; y_step = 1;
    }

    int i_min, i_max, i_step;
    if (i_rev) {
        i_min = 7; i_max = -1; i_step = -1;
    }
    else {
        i_min = 0; i_max = 8; i_step = 1;
    }

    unsigned sum = 0;
    uint8_t xor = 0;
    for (int k = y_min; k != y_max; k += y_step) {
        u_int8_t data = msg[k];
        for (int bit = i_min; bit != i_max; bit += i_step) {
            if ((data >> bit) & 1) {
                sum += key;
                xor ^= key;

                if (rev) {
                    key = (key << 1) ^ gen;
                } else {
                    key = (key >> 1) ^ gen;
                }
            } else {
                if (rev) {
                    key = (key << 1);
                }
                else {
                    key = (key >> 1);
                }
            }
        }
    }
    *sum_add = sum;
    //*sum_add = sum | (sum >> 8);
    *sum_xor = xor;
}

// Checksum is actually an "LFSR-based Toeplitz hash"
// gen needs to includes the msb if the lfsr is rolling, key is the initial key
__attribute__((always_inline))
static inline void algo_lfsr_digest8_galois(int y_rev, int i_rev, int rev,
        uint8_t const *msg, int bytes, uint8_t gen, uint8_t key, uint8_t *sum_add, uint8_t *sum_xor)
{
    // all this will be optimized away
    int y_min, y_max, y_step;
    if (y_rev) {
        y_min = bytes - 1; y_max = -1; y_step = -1;
    } else {
        y_min = 0; y_max = bytes; y_step = 1;
    }

    int i_min, i_max, i_step;
    if (i_rev) {
        i_min = 7; i_max = -1; i_step = -1;
    }
    else {
        i_min = 0; i_max = 8; i_step = 1;
    }

    unsigned sum = 0;
    uint8_t xor = 0;
    // for (int k = bytes - 1; k >= 0; --k) {
    // for (int k = 0; k < bytes; ++k) {
    for (int k = y_min; k != y_max; k += y_step) {
        u_int8_t data = msg[k];
        // for (int bit = 7; bit >= 0; --bit) {
        // for (int bit = 0; bit <= 7; ++bit) {
        for (int bit = i_min; bit != i_max; bit += i_step) {
            // fprintf(stderr, "key at bit %d : %04x\n", bit, key);
            // if data bit is set then xor with key
            if ((data >> bit) & 1) {
                sum += key;
                xor ^= key;
            }

            // - Galois LFSR -
            // shift the key right (not roll, the lsb is dropped)
            // and apply the gen (needs to include the dropped lsb as msb)
            if (rev) {
                if (key & 0x80)
                    key = (key << 1) ^ gen;
                else
                    key = (key << 1);

            } else {
                if (key & 1)
                    key = (key >> 1) ^ gen;
                else
                    key = (key >> 1);
            }
        }
    }
    *sum_add = sum;
    //*sum_add = sum | (sum >> 8);
    *sum_xor = xor;
}

// Checksum is actually an "LFSR-based Toeplitz hash"
// gen needs to includes the msb if the lfsr is rolling, key is the initial key
__attribute__((always_inline))
static inline void algo_lfsr_digest8_fibonacci(int y_rev, int i_rev, int rev,
        uint8_t const *msg, int bytes, uint8_t gen, uint8_t key, uint8_t *sum_add, uint8_t *sum_xor)
{
    // all this will be optimized away
    int y_min, y_max, y_step;
    if (y_rev) {
        y_min = bytes - 1; y_max = -1; y_step = -1;
    } else {
        y_min = 0; y_max = bytes; y_step = 1;
    }

    int i_min, i_max, i_step;
    if (i_rev) {
        i_min = 7; i_max = -1; i_step = -1;
    }
    else {
        i_min = 0; i_max = 8; i_step = 1;
    }

    unsigned sum = 0;
    uint8_t xor  = 0;
    // for (int k = bytes - 1; k >= 0; --k) {
    // for (int k = 0; k < bytes; ++k) {
    for (int k = y_min; k != y_max; k += y_step) {
        u_int8_t data = msg[k];
        // for (int bit = 7; bit >= 0; --bit) {
        // for (int bit = 0; bit <= 7; ++bit) {
        for (int bit = i_min; bit != i_max; bit += i_step) {
            // fprintf(stderr, "key at bit %d : %04x\n", bit, key);
            // if data bit is set then xor with key
            if ((data >> bit) & 1) {
                sum += key;
                xor ^= key;
            }

            // - Fibonacci LFSR -
            // shift the key right (not roll, the lsb is dropped)
            // and set the msb to the parity of key and gen (needs to include lsb)
            if (rev) {
                if (parity(key & gen))
                    key = (key << 1) | (1 << 0);
                else
                    key = (key << 1);
            }
            else {
                if (parity(key & gen))
                    key = (key >> 1) | (1 << 7);
                else
                    key = (key >> 1);
            }
        }
    }
    *sum_add = sum;
    //*sum_add = sum | (sum >> 8);
    *sum_xor = xor;
}

__attribute__((always_inline))
static inline void call_algo(int algo, int y_rev, int i_rev, int rev,
        uint8_t const *msg, int bytes, uint8_t gen, uint8_t key, uint8_t *sum_add, uint8_t *sum_xor)
{
    if (algo == 1)
        algo_lfsr_digest8_galois(y_rev, i_rev, rev, msg, bytes, gen, key, sum_add, sum_xor);
    else if (algo == 2)
        algo_lfsr_digest8_fibonacci(y_rev, i_rev, rev, msg, bytes, gen, key, sum_add, sum_xor);
    else if (algo == 3)
        algo_fletcher8(y_rev, i_rev, rev, msg, bytes, gen, key, sum_add, sum_xor);
    else if (algo == 4)
        algo_shift16_8(y_rev, i_rev, rev, msg, bytes, gen, key, sum_add, sum_xor);
    else
        {}
}

#define DONE(msg, r, s) do { printf("Done with g %02x k %02x final XOR %02x using %s (%.0f %%)\n", g, k, r, msg, s); /*exit(0);*/ } while (0)

static struct data data[LIST_MAX];
static unsigned msg_len  = 0;
static unsigned list_len = 0;
static unsigned min_matches = 0;

__attribute__((always_inline))
static inline void *runner(int algo, int y_rev, int i_rev, int rev)
{
    for (unsigned g = 0; g <= 0xff; ++g) {
        for (unsigned k = 0; k <= 0xff; ++k) {
            struct data rd = data[0];
            uint8_t rs;
            uint8_t rx;
            call_algo(algo, y_rev, i_rev, rev, rd.d, msg_len, g, k, &rs, &rx);
            uint8_t rsx = rs ^ rd.chk;
            uint8_t rxx = rx ^ rd.chk;
            uint8_t rsa = rs + rd.chk;
            uint8_t rxa = rx + rd.chk;
            uint8_t rss = rs - rd.chk;
            uint8_t rxs = rx - rd.chk;
            //printf("g %02x k %02x chk %02x rsx: %02x rxx: %02x rsa: %02x rxa: %02x rss: %02x rxs: %02x\n", g, k, rd.chk, rsx, rxx, rsa, rxa, rss, rxs);
            //printf("rsx: %02x rxx: %02x rsa: %02x rxa: %02x rss: %02x rxs: %02x\n", rsx, rxx, rsa, rxa, rss, rxs);

            unsigned fsx = 0;
            unsigned fxx = 0;
            unsigned fsa = 0;
            unsigned fxa = 0;
            unsigned fss = 0;
            unsigned fxs = 0;

            for (unsigned i = 1; i < list_len; ++i) {
                struct data dd = data[i];
                uint8_t ds;
                uint8_t dx;
                call_algo(algo, y_rev, i_rev, rev, dd.d, msg_len, g, k, &ds, &dx);
                uint8_t dsx = ds ^ dd.chk;
                uint8_t dxx = dx ^ dd.chk;
                uint8_t dsa = ds + dd.chk;
                uint8_t dxa = dx + dd.chk;
                uint8_t dss = ds - dd.chk;
                uint8_t dxs = dx - dd.chk;
                //printf("dsx: %02x dxx: %02x dsa: %02x dxa: %02x dss: %02x dxs: %02x\n", dsx, dxx, dsa, dxa, dss, dxs);

                fsx += rsx == dsx;
                fxx += rxx == dxx;
                fsa += rsa == dsa;
                fxa += rxa == dxa;
                fss += rss == dss;
                fxs += rxs == dxs;
            }

            if (fsx >= min_matches) DONE("sum xor", rsx, 100.0 * fsx / (list_len - 1));
            if (fsx >= min_matches) DONE("sum xor", rsx, 100.0 * fsx / (list_len - 1));
            if (fxx >= min_matches) DONE("xor xor", rxx, 100.0 * fxx / (list_len - 1));
            if (fsa >= min_matches) DONE("sum add", rsa, 100.0 * fsa / (list_len - 1));
            if (fxa >= min_matches) DONE("xor add", rxa, 100.0 * fxa / (list_len - 1));
            if (fss >= min_matches) DONE("sum sub", rss, 100.0 * fss / (list_len - 1));
            if (fxs >= min_matches) DONE("xor sub", rxs, 100.0 * fxs / (list_len - 1));
        }
    }

    return NULL;
}

static int job_run(int job_num)
{
    // unroll every code path
    if (job_num == 0)
        MEASURE("Galois ", runner(1, 0, 0, 0););
    else if (job_num == 1)
        MEASURE("Galois BYTE_REFLECT", runner(1, 1, 0, 0););
    else if (job_num == 2)
        MEASURE("Galois BIT_REFLECT", runner(1, 0, 1, 0););
    else if (job_num == 3)
        MEASURE("Galois BIT_REFLECT BYTE_REFLECT", runner(1, 1, 1, 0););
    else if (job_num == 4)
        MEASURE("Rev-Galois ", runner(1, 0, 0, 1););
    else if (job_num == 5)
        MEASURE("Rev-Galois BYTE_REFLECT", runner(1, 1, 0, 1););
    else if (job_num == 6)
        MEASURE("Rev-Galois BIT_REFLECT", runner(1, 0, 1, 1););
    else if (job_num == 7)
        MEASURE("Rev-Galois BIT_REFLECT BYTE_REFLECT", runner(1, 1, 1, 1););

    else if (job_num == 8)
        MEASURE("Fibonacci ", runner(2, 0, 0, 0););
    else if (job_num == 9)
        MEASURE("Fibonacci BYTE_REFLECT", runner(2, 1, 0, 0););
    else if (job_num == 10)
        MEASURE("Fibonacci BIT_REFLECT", runner(2, 0, 1, 0););
    else if (job_num == 11)
        MEASURE("Fibonacci BIT_REFLECT BYTE_REFLECT", runner(2, 1, 1, 0););
    else if (job_num == 12)
        MEASURE("Rev-Fibonacci ", runner(2, 0, 0, 1););
    else if (job_num == 13)
        MEASURE("Rev-Fibonacci BYTE_REFLECT", runner(2, 1, 0, 1););
    else if (job_num == 14)
        MEASURE("Rev-Fibonacci BIT_REFLECT", runner(2, 0, 1, 1););
    else if (job_num == 15)
        MEASURE("Rev-Fibonacci BIT_REFLECT BYTE_REFLECT", runner(2, 1, 1, 1););

    else if (job_num == 16)
        MEASURE("Fletcher ", runner(3, 0, 0, 0););
    else if (job_num == 17)
        MEASURE("Fletcher BYTE_REFLECT", runner(3, 1, 0, 0););
    else if (job_num == 18)
        MEASURE("Fletcher BIT_REFLECT", runner(3, 0, 1, 0););
    else if (job_num == 19)
        MEASURE("Fletcher BIT_REFLECT BYTE_REFLECT", runner(3, 1, 1, 0););
    else if (job_num == 20)
        MEASURE("Rev-Fletcher ", runner(3, 0, 0, 1););
    else if (job_num == 21)
        MEASURE("Rev-Fletcher BYTE_REFLECT", runner(3, 1, 0, 1););
    else if (job_num == 22)
        MEASURE("Rev-Fletcher BIT_REFLECT", runner(3, 0, 1, 1););
    else if (job_num == 23)
        MEASURE("Rev-Fletcher BIT_REFLECT BYTE_REFLECT", runner(3, 1, 1, 1););

    else if (job_num == 24)
        MEASURE("Shift16 ", runner(4, 0, 0, 0););
    else if (job_num == 25)
        MEASURE("Shift16 BYTE_REFLECT", runner(4, 1, 0, 0););
    else if (job_num == 26)
        MEASURE("Shift16 BIT_REFLECT", runner(4, 0, 1, 0););
    else if (job_num == 27)
        MEASURE("Shift16 BIT_REFLECT BYTE_REFLECT", runner(4, 1, 1, 0););
    else if (job_num == 28)
        MEASURE("Rev-Shift16 ", runner(4, 0, 0, 1););
    else if (job_num == 29)
        MEASURE("Rev-Shift16 BYTE_REFLECT", runner(4, 1, 0, 1););
    else if (job_num == 30)
        MEASURE("Rev-Shift16 BIT_REFLECT", runner(4, 0, 1, 1););
    else if (job_num == 31)
        MEASURE("Rev-Shift16 BIT_REFLECT BYTE_REFLECT", runner(4, 1, 1, 1););
    else
        {}

    return 32;
}

// e.g. Ambient Weather F007TH Thermo-Hygrometer
// e.g. Acurite-606TX

__attribute__((noreturn))
static void usage(int argc, char const *argv[])
{
    (void)argc;
    fprintf(stderr, "%s: [-v] [-s|-p] codes.txt\n", argv[0]);
    exit(1);
}

int main(int argc, char const *argv[])
{
    start_runtimes();

    int verbose = 0;
    int parallel = 0;
    double min_matches_pct = 0.8;

    int i = 1;
    for (; i < argc; ++i) {
        if (*argv[i] != '-')
            break;
        if (argv[i][1] == 'h')
            usage(argc, argv);
        else if (argv[i][1] == 'v')
            verbose++;
        else if (argv[i][1] == 's')
            parallel = 0;
        else if (argv[i][1] == 'p')
            parallel = 1;
        else {
            fprintf(stderr, "Wrong argument (%s).\n", argv[i]);
            usage(argc, argv);
        }
    }

    if (argc <= i) {
        fprintf(stderr, "Reading STDIN...\n");
    }
    int ret = read_codes(argv[i], data, &msg_len, MSG_MAX, LIST_MAX);
    if (ret <= 0) {
        fprintf(stderr, "Missing data!\n");
        usage(argc, argv);
    }
    list_len = (unsigned)ret;
    if (msg_len <= 1) {
        fprintf(stderr, "Message length too short!\n");
        usage(argc, argv);
    }
    if (verbose)
        print_codes(data, msg_len, list_len);

    fprintf(stderr, "Processing...\n");
    min_matches = list_len * min_matches_pct;
    msg_len -= 1; // use 8-bit chk

    if (parallel)
        job_exec_parallel(job_run, 0);
    else
        job_exec_sequential(job_run);

    print_runtimes();
}
