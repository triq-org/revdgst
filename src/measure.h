/** @file
    measure.h: macro to measure and print execution time

    Copyright (C) 2019 Christian W. Zuckschwerdt <zany@triq.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
*/

#include <time.h>
#include <sys/times.h>
#include <unistd.h>

/// Measure the CPU time used by the process, not the wall-clock time.
#define MEASURE_CPU(label, block)                                              \
    do {                                                                   \
        clock_t start = clock();                                           \
        block;                                                             \
        clock_t stop   = clock();                                          \
        double elapsed = (double)(stop - start) / CLOCKS_PER_SEC;          \
        printf("Time elapsed in s: %.2f for: %s\n\n", elapsed, label);     \
    } while (0)

/// Measure wall clock time.
#define MEASURE(label, block)                                         \
    do {                                                                   \
        struct timespec start, finish;                                     \
        clock_gettime(CLOCK_MONOTONIC, &start);                            \
        block;                                                             \
        clock_gettime(CLOCK_MONOTONIC, &finish);                           \
        double elapsed = (finish.tv_sec - start.tv_sec);                   \
        elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;        \
        printf("Time elapsed in s: %.2f for: %s\n\n", elapsed, label);     \
    } while (0)

static clock_t start_time_real;

/// Capture overall process time used, in the style of "time" command.
static void start_runtimes()
{
    struct tms tms;
    start_time_real = times(&tms);
}

/// Print overall process time used, in the style of "time" command.
static void print_runtimes()
{
    struct tms tms;
    clock_t time_real = times(&tms);
    // clock_t is 1/CLK_TCK seconds; CLK_TCK is defined as the return value of
    int clk_tck = sysconf(_SC_CLK_TCK);
    double pct = (double)(tms.tms_utime + tms.tms_stime) / (time_real - start_time_real);
    printf("Run time: %.2fs user %.2fs system %.0f%% cpu %.3f total\n\n",
            (double)tms.tms_utime / clk_tck,
            (double)tms.tms_stime / clk_tck,
            pct * 100.0,
            (double)(time_real - start_time_real) / clk_tck);
}

// s.a. pthread_getcpuclockid(3)
