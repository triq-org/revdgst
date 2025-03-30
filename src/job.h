/** @file
    job.h: run jobs in sequence or parallel.

    Copyright (C) 2019 Christian W. Zuckschwerdt <zany@triq.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
*/

#include <stdlib.h>
#include <stdio.h>

#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_CPUS 128

static int job_count;
static _Atomic int job_next;
static int (*job_call)(int);

static void *job_runner(void *args)
{
    for (;;) {
        int job = atomic_fetch_add(&job_next, 1);

        if (job >= job_count)
            return NULL;

        job_call(job);
    }
}

static void job_exec_sequential(int (*call)(int))
{
    int count = call(-1);
    for (int i = 0; i < count; ++i) {
        call(i);
    }
}

static int job_default_thread_count(void)
{
    return sysconf(_SC_NPROCESSORS_ONLN); // Get the number of logical CPUs.
}

static void job_exec_parallel(int (*call)(int), int numofthreads)
{
    pthread_t thread[MAX_CPUS]; // numofthreads
    int args[MAX_CPUS];         // numofthreads

    if (numofthreads <= 0)
        numofthreads = job_default_thread_count();
    fprintf(stderr, "Running %d threads...\n", numofthreads);

    job_count = call(-1);
    atomic_store_explicit(&job_next, 0, memory_order_relaxed);
    job_call = call;

    for (int i = 0; i < numofthreads; ++i) {
        args[i] = i;
        int ret = pthread_create(&thread[i], NULL, job_runner, (void *)&args[i]);
        if (ret) {
            perror("job_exec_parallel pthread_create");
            exit(1);
        }
    }

    for (int i = 0; i < numofthreads; ++i) {
        pthread_join(thread[i], NULL);
    }
}
