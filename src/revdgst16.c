/** @file
    revdgst16: reverse 16-bit LFSR digest.

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
Modify this algorithm to test other possible checksum methods.
*/

__attribute__((always_inline))
static inline void algo_lfsr_digest16(uint8_t *msg, unsigned bytes, uint16_t gen, uint16_t key, uint16_t *sum, uint16_t *xor)
{
    *sum = 0;
    *xor = 0;
    for (unsigned k = 0; k < bytes; ++k) {
        uint8_t data = msg[k];
        for (int bit = 7; bit >= 0; --bit) {
            // fprintf(stderr, "key at bit %d : %04x\n", bit, key);
            // if data bit is set then xor with key
            if ((data >> bit) & 1) {
                *sum += key;
                *xor ^= key;
            }

            // roll the key right (actually the lsb is dropped here)
            // and apply the gen (needs to include the dropped lsb as msb)
            if (key & 1)
                key = (key >> 1) ^ gen;
            else
                key = (key >> 1);

            // equivalent but slower on O3:
            // key = (key >> 1) ^ ((-(key & 1)) & gen);
            // also slower:
            // key = (key >> 1) ^ ((key & 1) * gen);

            // -or- (Fibonacci LFSR)
            //if (parity(key & gen))
            //    key = (key >> 1) | (1 << 15);
            //else
            //    key = (key >> 1);
        }
    }
}

#define DONE(fin, msg) do { ++found; printf("Done with g %04x k %04x final %04x using %s\n", g, k, fin, msg); } while (0)

static struct data data[LIST_MAX];
static unsigned msg_len  = 0;
static unsigned list_len = 0;

static int found = 0;
static int numofthreads = 1;

static int runner(int offset)
{
    if (offset < 0 || offset > numofthreads)
        return numofthreads;
    int step = numofthreads;

    for (unsigned g=0x8000 + offset; !found && g <= 0xffff; g += step) {
        for (unsigned k = 0; k <= 0xffff; ++k) {
            struct data rd = data[0];
            uint16_t rs;
            uint16_t rx;
            algo_lfsr_digest16(rd.d, msg_len, g, k, &rs, &rx);
            uint16_t rsx = rs ^ rd.chk16;
            uint16_t rxx = rx ^ rd.chk16;
            uint16_t rsa = rs + rd.chk16;
            uint16_t rxa = rx + rd.chk16;
            uint16_t rss = rs - rd.chk16;
            uint16_t rxs = rx - rd.chk16;
            //printf("g %04x k %04x chk %04x rsx: %04x rxx: %04x rsa: %04x rxa: %04x rss: %04x rxs: %04x\n", g, k, rd.chk16, rsx, rxx, rsa, rxa, rss, rxs);
            //printf("rsx: %04x rxx: %04x rsa: %04x rxa: %04x rss: %04x rxs: %04x\n", rsx, rxx, rsa, rxa, rss, rxs);

            int fsx = 1;
            int fxx = 1;
            int fsa = 1;
            int fxa = 1;
            int fss = 1;
            int fxs = 1;

            for (int i = 1; i < list_len; ++i) {
                struct data dd = data[i];
                uint16_t ds;
                uint16_t dx;
                algo_lfsr_digest16(dd.d, msg_len, g, k, &ds, &dx);
                uint16_t dsx = ds ^ dd.chk16;
                uint16_t dxx = dx ^ dd.chk16;
                uint16_t dsa = ds + dd.chk16;
                uint16_t dxa = dx + dd.chk16;
                uint16_t dss = ds - dd.chk16;
                uint16_t dxs = dx - dd.chk16;
                //printf("dsx: %04x dxx: %04x dsa: %04x dxa: %04x dss: %04x dxs: %04x\n", dsx, dxx, dsa, dxa, dss, dxs);

                fsx &= rsx == dsx;
                fxx &= rxx == dxx;
                fsa &= rsa == dsa;
                fxa &= rxa == dxa;
                fss &= rss == dss;
                fxs &= rxs == dxs;

                if (fsx || fxx || fsa || fxa || fss || fxs) {
                    // still going
                }
                else {
                    // give up
                    break;
                }
            }

            if (fsx) DONE(rsx, "sum xor");
            if (fxx) DONE(rxx, "xor xor");
            if (fsa) DONE(rsa, "sum add");
            if (fxa) DONE(rxa, "xor add");
            if (fss) DONE(rss, "sum sub");
            if (fxs) DONE(rxs, "xor sub");
        }
    }

    return numofthreads;
}

// e.g. Maverick-ET73x
// e.g. TFA-303196

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

    if (argc <= 1) {
        fprintf(stderr, "Reading STDIN...\n");
    }
    list_len = read_codes(argv[1], data, &msg_len, MSG_MAX, LIST_MAX);
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

    fprintf(stderr, "Processing...\n");
    msg_len -= 2; // use 16-bit chk

    numofthreads = job_default_thread_count();
    job_exec_parallel(runner, numofthreads);

    // byte swap and run again
    fprintf(stderr, "Swapping byte order\n");
    for (int i = 0; i < list_len; ++i) {
        data[i].chk16 = (data[i].chk16 << 8) | (data[i].chk16 >> 8);
    }
    found = 0;
    job_exec_parallel(runner, numofthreads);

    print_runtimes();
}
