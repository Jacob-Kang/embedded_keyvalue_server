#ifndef __BWORKER_H
#define __BWORKER_H

#include <pthread.h>
#include <semaphore.h>
#include "server.h"

#define BPB_TIERING_FLASH 0
#define BPB_TCP_WORKER 1
#define MAX_BWORKER 10
#define NUM_BWORKER 2

#define THREAD_STACK_SIZE (1024 * 1024 * 4)

extern pthread_t bworker_threads[MAX_BWORKER];
extern pthread_mutex_t bworker_mutex;
extern sem_t *sem_bworker;

void bworkerInit(void);
#endif