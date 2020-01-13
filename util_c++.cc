#include <limits>
#include <string>
#include "util_c++.h"
extern "C" {
void *flashCacheCreate(const char *_path, struct kvDb *db,
                       const uint64_t max_size, const uint32_t max_file_size);

void *flashCacheCreate(
    const char *_path, struct kvDb *db,
    const uint64_t max_size = std::numeric_limits<uint64_t>::max(),
    const uint32_t max_file_size = static_cast<uint32_t>(12 * 1024 * 1024 *
                                                         0.125)) {
  Env *env = Env::Default();
  std::string path(_path);
  PersistentCacheConfig opt(env, path, max_size, log);
  // To avoid exceeding max_file_size. (cause of write buffer)
  //   opt.cache_file_size = max_file_size;
  opt.cache_file_size = max_file_size;
  opt.max_write_pipeline_backlog_size = std::numeric_limits<uint64_t>::max();
  opt.writer_qdepth = 1;
  opt.enable_direct_reads = false;
  opt.pipeline_writes = false;
  opt.write_buffer_size = 1 * 1024 * 1024;
  opt.writer_dispatch_size = opt.write_buffer_size;
  PersistentCacheTier *ncache = new NVMCache(opt, redis_db_);
  Status s = ncache->Open();
  assert(s.ok());
  return (void *)ncache;
}

void NVMCacheDestroy(void *nvmc) {
  PersistentTieredCache *nvmcPtr = (PersistentTieredCache *)nvmc;
  //   nvmcPtr->Close();
  delete nvmcPtr;
}

int NVMCacheInsert(void *nvmc, const char *_key, const char *data,
                   const size_t size) {
  Status s;
  NVMCache *cache_ = (NVMCache *)nvmc;
  std::string Key(_key);

  s = cache_->Insert(Key, data, size);
  if (!s.ok()) return REDIS_ERR;
  return REDIS_OK;
}

int NVMCacheInsertWithRobj(void *nvmc, const char *_key, const char *data,
                           const size_t size, robj *val_obj) {
  Status s;
  NVMCache *cache_ = (NVMCache *)nvmc;
  std::string Key(_key);

  s = cache_->Insert(Key, data, size, val_obj);
  if (!s.ok()) return REDIS_ERR;
  return REDIS_OK;
}

char *NVMCacheLookup(void *nvmc, const char *_key, size_t *size) {
  NVMCache *cache_ = (NVMCache *)nvmc;
  std::string Key(_key);
  return cache_->Lookup(Key, size);
}

int NVMCacheErase(void *nvmc, const char *_key) {
  NVMCache *cache_ = (NVMCache *)nvmc;
  std::string Key(_key);
  return cache_->Erase(Key);
}

bool NVMSetMetakey(redisDb *db, sds key, sds lba) {
  NVMCache *cache_ = (NVMCache *)db->nvmcache;
  return cache_->InsertMetaData(key, lba);
}

bool NVMDeleteMetakey(redisDb *db, sds key) {
  NVMCache *cache_ = (NVMCache *)db->nvmcache;
  return cache_->DeleteMetaData(key);
}

char *NVMCachePrintStats(void *nvmc) {
  NVMCache *cache_ = (NVMCache *)nvmc;
  std::string stat = cache_->PrintStats();
  void *stats = zmalloc(stat.size());
  memcpy(stats, stat.c_str(), stat.size());
  return (sds)stats;
}

void NVMCacheTest(redisDb *db) {
  void *cache_ = db->nvmcache;
  Random rnd(301);
  //   std::string value;
  char buf[100];
  for (int i = 000000; i < 30000; i++) {
    snprintf(buf, sizeof(buf), "key%06d", i);
    std::string tmp_value(buf);
    for (int j = 0; j < 991; ++j) {
      tmp_value += rnd.Uniform(26) + 'a';
    }
    char *c_val = new char[tmp_value.size() + 1];
    std::copy(tmp_value.begin(), tmp_value.end(), c_val);
    c_val[tmp_value.size()] = '\0';
    robj *new_val = createStringObject(c_val, tmp_value.size() + 1);
    new_val->where = REDIS_LOC_NVM;
    char *dataValue = (char *)new_val->ptr;
    // NVMCacheInsert(cache_, buf, (char *)new_val, sizeof(robj*) +
    // strlen(dataValue));
    NVMCacheInsert(cache_, buf, c_val, strlen(dataValue) + 1);
  }

  snprintf(buf, sizeof(buf), "key%06d", 0);
  size_t size;
  NVMCacheLookup(cache_, buf, &size);
  //   if(NVMCacheErase(cache_, buf))
  //     std::cout << "NVMCacheErase Success" << std::endl;
  //   else
  //     std::cout << "NVMCacheErase Fail" << std::endl;
  NVMCacheErase(cache_, buf);
  NVMCacheLookup(cache_, buf, &size);
  for (int i = 1462; i < 30000; i++) {
    snprintf(buf, sizeof(buf), "key%06d", i);
    char *val;
    // size_t size;
    if ((val = NVMCacheLookup(cache_, buf, &size)) != NULL) {
      std::cout << buf << ": " << val << std::endl;

      // } else{
      //     robj *key_obj = createStringObject(buf, strlen(buf));
      //     robj *val_obj = loadDataFromRocksDbAndAddReply_NVM(db, key_obj);
      //     std::cout << (sds)key_obj->ptr << ": " << (sds)val_obj->ptr <<
      //     std::endl;
    }
  }
  std::cout << "hello" << std::endl;
  NVMCachePrintStats(cache_);

  NVMCacheDestroy(cache_);
}
}