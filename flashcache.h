#ifndef __FLASHCACHE_H
#define __FLASHCACHE_H

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <boost/crc.hpp>
#include <cerrno>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "my_lru.hpp"
#include "server.h"
#include "util.h"
#include "util_c++.h"

int FileExists(const std::string& fname);
int GetMultiFiles(const std::string& dir, std::vector<std::string>* result);

typedef struct LogicalBlockAddress {
  LogicalBlockAddress() {}
  LogicalBlockAddress(const uint32_t cache_id, const uint32_t off,
                      const uint16_t size)
      : cache_id_(cache_id), off_(off), size_(size) {}

  uint32_t cache_id_ = 0;
  uint32_t off_ = 0;
  uint32_t size_ = 0;
} LBA;
class CacheWriteBuffer {
 public:
  CacheWriteBuffer(const size_t size = 1ULL * 1024 * 1024)
      : size_(size), pos_(0) {
    buf_.reset(new char[size_]);
    // assert(!pos_);
    // assert(size_);
  }
  ~CacheWriteBuffer() {}
  void Append(const char* buf, const size_t size) {
    // assert(pos_ + size <= size_);
    memcpy(buf_.get() + pos_, buf, size);
    pos_ += size;
    // assert(pos_ <= size_);
  }
  void Reset() { pos_ = 0; }
  size_t Free() const { return size_ - pos_; }
  size_t Capacity() const { return size_; }
  size_t Used() const { return pos_; }
  char* Data() const { return buf_.get(); }

 private:
  std::unique_ptr<char[]> buf_;

  // 버퍼 max 공간
  const size_t size_;
  // 새로 써질 위치 (offset)
  size_t pos_;
};
struct CacheRecordHeader {
  CacheRecordHeader() : magic_(0), crc_(0), key_size_(0), val_size_(0) {}
  CacheRecordHeader(const uint32_t magic, const uint32_t key_size,
                    const uint32_t val_size)
      : magic_(magic), crc_(0), key_size_(key_size), val_size_(val_size) {}
  uint32_t magic_;
  uint32_t crc_;
  uint32_t key_size_;
  uint32_t val_size_;
};
struct CacheRecord {
  CacheRecord() {}
  CacheRecord(const std::string& key, const std::string& val)
      : hdr_(MAGIC, static_cast<uint32_t>(key.size()),
             static_cast<uint32_t>(val.size())),
        key_(key),
        val_(val) {
    hdr_.crc_ = ComputeCRC();
  }
  uint32_t ComputeCRC() const;
  static uint32_t CalcSize(const std::string& key, const std::string& val) {
    return static_cast<uint32_t>(sizeof(CacheRecordHeader) + key.size() +
                                 val.size());
  }
  bool Serialize(std::vector<CacheWriteBuffer*>* bufs, size_t* woff);
  bool Deserialize(const char* data, size_t read_size);
  bool Append(std::vector<CacheWriteBuffer*>* bufs, size_t* woff,
              const char* data, const size_t size);

  static const uint32_t MAGIC = 0xfefa;
  CacheRecordHeader hdr_;
  std::string key_;
  std::string val_;
};

// struct BlockInfo {
//  BlockInfo(const std::string& key, const LBA& lba = LBA())
//      : key_(key, lba_(lba) {}
//
//  std::string key_;
//  LBA lba_;
//};

struct FlashCacheConfig {
  // Default: 4M
  uint32_t cache_file_size = 4ULL * 1024 * 1024;
  // Default: 1M
  uint32_t write_buffer_size = 1ULL * 1024 * 1024;
  std::string path;
  uint64_t cache_size;
};
class WriteableCacheFile {
 public:
  WriteableCacheFile(const std::string& dir, const uint32_t cache_id,
                     const uint32_t max_size, const uint32_t write_buffer_size,
                     const void* cache)
      : dir_(dir),
        cache_id_(cache_id),
        write_buffer_size_(write_buffer_size),
        max_size_(max_size),
        cache_(const_cast<void*>(cache)) {}
  ~WriteableCacheFile() {
    Close();
    chLog(LOG_NOTICE, "[FlashCache] %d.rc ~WriteableCacheFile()", cacheid());
  }
  bool Create();
  int NewWritableFile(const std::string& fname);
  bool Open();
  void Close();
  int Delete();
  void Repair(void* metadata_);
  bool ExpandBuffer(const size_t size);
  void DispatchBuffer();
  void Write();
  void Write(const char* src, int size_);
  static void* writerJobs(void* arg);
  bool Append(const std::string& key, const std::string& val, LBA* lba);
  bool Read(const LBA& lba, std::string* key, std::string* block,
            char* scratch);
  std::string Path() const {
    return dir_ + "/" + std::to_string(cache_id_) + ".rc";
  }
  std::string Path(uint32_t _cache_id) const {
    return dir_ + "/" + std::to_string(_cache_id) + ".rc";
  }
  int cacheid() { return cache_id_; }
  int getFilesize() { return filesize_; }
  bool Eof() const { return eof_; }
  std::list<std::string>& key_lists() { return key_lists_; }

 private:
  void* cache_;
  bool ReadBuffer(const LBA& lba, char* scratch);
  void ClearBuffers();
  static const size_t kFileAlignmentSize = 4 * 1024;  // align file size
  const std::string dir_;                             // Directory name
  const uint32_t cache_id_;                           // Cache id for the file
  const uint32_t max_size_;  // 캐시 파일 최종 크기
  const uint32_t write_buffer_size_;

  uint32_t filesize_ = 0;  // 현재까지 써진 파일 크기
  int fd_;
  // std::unique_ptr<WriteableCacheFile> file_;
  std::list<std::string> key_lists_;
  bool eof_ = false;
  uint32_t disk_woff_ = 0;               // Offset to write on disk
  std::vector<CacheWriteBuffer*> bufs_;  // Written buffers
  size_t buf_woff_ =
      0;  // 써야할 cache buffer indx, 0이면 아직 하나도 생성 안한것임.
  size_t buf_doff_ = 0;  // dispatch 해야할 cache buffer index
  bool is_writer_running = false;  // bg로 버퍼에서 실제파일로 기록중인 작업 수
#ifdef __APPLE__
  sem_t* sem_writer;
#else
  sem_t sem_writer;

#endif
  std::thread tid;
};

class Metadata {
 public:
  Metadata() {}
  ~Metadata() {}

  // Insert a given cache file
  bool Insert(const uint32_t cache_id, WriteableCacheFile* file);
  void Insert(const std::string& key, const LBA& lba);
  bool Lookup(const std::string& key, LBA* lba);
  WriteableCacheFile* Lookup(const uint32_t cache_id);
  void Erase(const std::string& key);
  WriteableCacheFile* Evict();
  // Lookup cache file based on cache_id

  // Lookup block information from block index
  // 강제 종료를 위해서 이것만 public (FlashCache::Close() 참고)
  lru_cache<int, WriteableCacheFile*> cache_file_index_;

 private:
  std::unordered_map<std::string, LBA> block_index_;
};

class FlashCache {
 public:
  explicit FlashCache(const FlashCacheConfig& opt, struct kvDb* db)
      : opt_(opt), db_(db) {
    pthread_mutex_init(&fcache_mutex, NULL);
  };
  //  opt_.write_buffer_size, opt_.write_buffer_count());
  ~FlashCache() {
    chLog(LOG_NOTICE, "[FlashCache] ~FlashCache()");
    Close();
  }
  int Open();
  int Close();
  int Insert(const std::string& key, const std::string& data);
  char* Lookup(const std::string& key, size_t* size);
  bool Evict(const size_t size);
  void DeleteMetaData(std::string& key);
  // bool Reserve(const size_t size);

  int NewCacheFile();
  // int OpenCacheFile(uint32_t cache_id, int32_t thread_id = 0);

 private:
  pthread_mutex_t fcache_mutex;
  std::string key_;
  std::string data_;
  uint64_t size_;  // flash cache 전체 크기
  struct kvDb* db_;
  uint32_t writer_cache_id_ = 0;
  WriteableCacheFile* cache_file_;
  FlashCacheConfig opt_;
  Metadata metadata_;
};

#endif
