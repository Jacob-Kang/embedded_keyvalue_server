#ifndef __BWORKER_H
#define __BWORKER_H

#include <pthread.h>
#include "server.h"
#define NUM_BWORKER 6

#define BPB_TCP_WORKER 0
#define BPB_TIERING_SD 1

#define THREAD_STACK_SIZE (1024 * 1024 * 4)

static pthread_t bworker_threads[NUM_BWORKER];
pthread_mutex_t bworker_mutex[NUM_BWORKER];
pthread_cond_t bworker_cond[NUM_BWORKER];

void bworkerInit(void);
#endif