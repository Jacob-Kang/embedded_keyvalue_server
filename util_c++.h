#ifndef __UTIL_C___H_
#define __UTIL_C___H_

#include "server.h"
#ifdef __cplusplus
extern "C" {
#endif

// FlashCache
void *flashCacheCreate(const char *_path, struct kvDb *db,
                       const uint64_t max_size, const uint32_t max_file_size);
void flashCacheDestroy(void *fc);
int flashCacheInsert(void *fc, const char *_key, const char *data,
                     const size_t size);
char *flashCacheLookup(void *fc, const char *_key, size_t *size);
int flashCacheErase(void *fc, const char *_key);

void *PQCreate(int max);
void PQDestroy(void *pq);
void PQinitQueue(void *pq);
void PQEnqueue(void *pq, struct hashEntry *entry, int mode);
void PQEnqueueWithKey(void *pq, struct hashEntry *entry, int32_t key, int mode);
struct hashEntry *PQDequeue(void *pq, int mode);
int PQsize(void *pq);
int PQisFull(void *pq);
int PQMaxsize(void *pq);
struct hashEntry *PQlookup(void *pq, struct hashEntry *entry);
bool PQremove(void *pq, struct hashEntry *entry, int mode);
struct hashEntry *PQlookupAndPop(void *pq, int32_t key, int mode);
#ifdef __cplusplus
}
#endif
#endif