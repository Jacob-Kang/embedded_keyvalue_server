#include <limits>
#include <queue>
#include "flashcache.h"
#include "my_lru.hpp"
#include "util_c++.h"
extern "C" {

void *LRUCacheCreate(struct kvDb *db) {
  void *mem_lru =
      static_cast<void *>(new lru_cache<std::string, struct hashEntry *>);
  return mem_lru;
}
void LRUCacheDestroy(void *mlru) {
  lru_cache<std::string, struct hashEntry *> *cache_ =
      static_cast<lru_cache<std::string, struct hashEntry *> *>(mlru);
  delete cache_;
  return;
}
int LRUCacheInsert(void *mlru, char *_key, struct hashEntry *_val) {
  lru_cache<std::string, struct hashEntry *> *cache_ =
      static_cast<lru_cache<std::string, struct hashEntry *> *>(mlru);
  std::string key_(_key);
  cache_->insert(key_, _val);
  return 0;
}
struct hashEntry *LRUCacheGet(void *mlru, char *_key) {
  lru_cache<std::string, struct hashEntry *> *cache_ =
      (lru_cache<std::string, struct hashEntry *> *)mlru;
  std::string key_(_key);
  return cache_->get(key_);
}
struct hashEntry *LRUCacheEvict(void *mlru) {
  lru_cache<std::string, struct hashEntry *> *cache_ =
      (lru_cache<std::string, struct hashEntry *> *)mlru;
  if (!cache_->empty()) return cache_->evict();
  return NULL;
}
void LRUCacheErase(void *mlru, char *_key) {
  lru_cache<std::string, struct hashEntry *> *cache_ =
      (lru_cache<std::string, struct hashEntry *> *)mlru;
  std::string key_(_key);
  cache_->erase(key_);
}

int LRUCacheSize(void *mlru) {
  lru_cache<std::string, struct hashEntry *> *cache_ =
      (lru_cache<std::string, struct hashEntry *> *)mlru;
  if (!cache_->empty()) return cache_->size();
  return 0;
}

void *QueueCreate(struct kvDb *db) {
  void *q = static_cast<void *>(new std::queue<struct hashEntry *>);
  return q;
}
void QueueEnqueue(void *mq, struct hashEntry *_val) {
  std::queue<struct hashEntry *> *q =
      static_cast<std::queue<struct hashEntry *> *>(mq);
  q->push(_val);
}
struct hashEntry *QueueDequeue(void *mq) {
  std::queue<struct hashEntry *> *q =
      static_cast<std::queue<struct hashEntry *> *>(mq);
  struct hashEntry *ret = q->front();
  q->pop();
  return ret;
}

int QueueSize(void *mq) {
  std::queue<struct hashEntry *> *q =
      static_cast<std::queue<struct hashEntry *> *>(mq);
  if (!q->empty()) return q->size();
  return 0;
}

void *flashCacheCreate(
    const char *_path, struct kvDb *db,
    const uint64_t max_size = std::numeric_limits<uint64_t>::max(),
    const uint32_t max_file_size = static_cast<uint32_t>(4 * 1024 * 1024)) {
  std::string path(_path);
  FlashCacheConfig opt;
  opt.path = path;
  opt.cache_size = max_size;
  opt.cache_file_size = max_file_size;
  opt.write_buffer_size = server.flashCache_writebuffer_size;
  FlashCache *fcache = new FlashCache(opt, db);
  if (fcache->Open()) {
#ifndef WITHOUT_LOG
    chLog(LOG_ERROR, "flash_cache open error");
#endif
    exit(-1);
  }
  return static_cast<void *>(fcache);
}
void flashCacheDestroy(void *fc) {
  FlashCache *cache_ = (FlashCache *)fc;
  delete cache_;
}
int flashCacheInsert(void *fc, const char *_key, const char *_data,
                     const size_t size) {
  FlashCache *cache_ = reinterpret_cast<FlashCache *>(fc);
  std::string Key(_key);
  std::string Val(_data);
  if (cache_->Insert(Key, _data)) {
#ifndef WITHOUT_LOG
    chLog(LOG_ERROR, "flash_cache insert error");
#endif
    return -1;
  }
  return 0;
}
char *flashCacheLookup(void *fc, const char *_key, size_t *size) {
  FlashCache *cache_ = (FlashCache *)fc;
  std::string Key(_key);
  return cache_->Lookup(Key, size);
}
void flashCacheMetakey(void *fc, const char *_key) {
  FlashCache *cache_ = (FlashCache *)fc;
  std::string Key(_key);
  cache_->DeleteMetaData(Key);
}
// extern "C"
}