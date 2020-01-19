#include "flashcache.h"

int FileExists(const std::string& fname) {
  int result = access(fname.c_str(), F_OK);
  if (result == 0) {
    return 0;
  }
  switch (errno) {
    case EACCES:
    case ELOOP:
    case ENAMETOOLONG:
    case ENOENT:
    case ENOTDIR:
      return 1;
    default:
#ifndef WITHOUT_LOG
      chLog(LOG_ERROR, "Unexpected error(%s) accessing file %s", result,
            fname.c_str());
#endif
      break;
  }
  return 0;
}

int GetMultiFiles(const std::string& dir, std::vector<std::string>* result) {
  result->clear();
  DIR* d = opendir(dir.c_str());
  if (d == nullptr) {
    return -1;
  }
  struct dirent* entry;
  while ((entry = readdir(d)) != nullptr) {
    result->push_back(entry->d_name);
  }
  closedir(d);
  return 0;
}

uint32_t CacheRecord::ComputeCRC() const {
  boost::crc_32_type crc;
  CacheRecordHeader tmp = hdr_;
  tmp.crc_ = 0;
  crc.process_bytes(reinterpret_cast<const char*>(&tmp), sizeof(tmp));
  crc.process_bytes(reinterpret_cast<const char*>(key_.data()), key_.size());
  crc.process_bytes(reinterpret_cast<const char*>(val_.data()), val_.size());
  return crc.checksum();
}

bool CacheRecord::Deserialize(const char* data, size_t read_size) {
  if (read_size < sizeof(CacheRecordHeader)) {
    return false;
  }
  memcpy(&hdr_, data, sizeof(hdr_));
  if (hdr_.key_size_ + hdr_.val_size_ + sizeof(hdr_) != read_size) {
    return false;
  }
  key_ = std::string(data + sizeof(hdr_), 0, hdr_.key_size_);
  val_ = std::string(data + sizeof(hdr_) + hdr_.key_size_, 0, hdr_.val_size_);

  if (!(hdr_.magic_ == MAGIC && ComputeCRC() == hdr_.crc_)) {
    fprintf(stderr, "** magic %d ** \n", hdr_.magic_);
    fprintf(stderr, "** key_size %d ** \n", hdr_.key_size_);
    fprintf(stderr, "** val_size %d ** \n", hdr_.val_size_);
    fprintf(stderr, "** key %s ** \n", key_.c_str());
    fprintf(stderr, "** val %s ** \n", val_.c_str());
    fprintf(stderr, "\n** cksum %d != %d **", hdr_.crc_, ComputeCRC());
    return false;
  }
  return true;
}
bool CacheRecord::Serialize(std::vector<CacheWriteBuffer*>* bufs,
                            size_t* woff) {
  return Append(bufs, woff, reinterpret_cast<const char*>(&hdr_),
                sizeof(hdr_)) &&
         Append(bufs, woff, key_.c_str(), key_.size()) &&
         Append(bufs, woff, val_.c_str(), val_.size());
}
bool CacheRecord::Append(std::vector<CacheWriteBuffer*>* bufs, size_t* woff,
                         const char* data, const size_t data_size) {
  const char* p = data;
  size_t size = data_size;
  while (size && *woff < bufs->size()) {
    CacheWriteBuffer* buf = (*bufs)[*woff];
    const size_t free = buf->Free();
    if (size <= free) {
      buf->Append(p, size);
      size = 0;
    } else {
      buf->Append(p, free);
      p += free;
      size -= free;
      assert(!buf->Free());
      assert(buf->Used() == buf->Capacity());
    }
    if (!buf->Free()) {
      *woff += 1;
    }
  }
  return !size;
}

int WriteableCacheFile::NewWritableFile(const std::string& fname) {
  fd_ = open(fname.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
  return 0;
}

bool WriteableCacheFile::Create() {
#ifndef WITHOUT_LOG
  chLog(LOG_NOTICE, "[FlashCache] Creating new cache %s (max size is %d KB)",
        Path().c_str(), max_size_ / 1024);
#endif
  if (!FileExists(Path())) {
#ifndef WITHOUT_LOG
    chLog(LOG_ERROR, "[FlashCache] File %s already exists.", Path().c_str());
#endif
  } else {
    fd_ = open(Path().c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
#ifdef __APPLE__
    char sem_name[15];
    sprintf(sem_name, "sem_writer_%d", cache_id_);
    sem_writer = sem_open(sem_name, O_CREAT, S_IRWXU, 0);
    if (sem_writer == SEM_FAILED) {
      chLog(LOG_ERROR, "unable to create semaphore");
      sem_unlink(sem_name);
      exit(-1);
    }
#else
    int ret = sem_init(sem_writer, 0, 0);
    if (ret == -1) {
      chLog(LOG_ERROR, "Writer semaphore init fail");
      exit(-1);
    }
#endif
    tid = std::thread(WriteableCacheFile::writerJobs, this);
  }
  return true;
}

bool WriteableCacheFile::Open() {
  // open(Path().c_str(), O_RDONLY);
  FILE* fp = fopen(Path().c_str(), "r");
  fseek(fp, 0, SEEK_END);
  filesize_ = ftell(fp);
  fd_ = fileno(fp);
  eof_ = true;
  return true;
  // rwlock_.AssertHeld();

  // ROCKS_LOG_DEBUG(log_, "Opening cache file %s", Path().c_str());
  // std::unique_ptr<RandomAccessFile> file;
  // Status status =
  //     NewRandomAccessCacheFile(env_, Path(), &file, enable_direct_reads);
  // if (!status.ok()) {
  //   Error(log_, "Error opening random access file %s. %s", Path().c_str(),
  //         status.ToString().c_str());
  //   return false;
  // }
  // // freader_.reset(new RandomAccessFileReader(std::move(file), Path(),
  // env_)); freader_.reset(new RandomAccessFileReader(std::move(file), env_));

  // return true;
}
void WriteableCacheFile::Close() {
#ifndef WITHOUT_LOG
  chLog(LOG_NOTICE, "Closing file %s. size=%d written=%d", Path().c_str(),
        filesize_, disk_woff_);
#endif
  if (tid.joinable()) {
    eof_ = true;
    // 강제 write를 위한 임시 증가
    if (server.flashcache_mode == MODE_FLASH_WRITEBUFFER) buf_woff_++;
    sem_post(sem_writer);
    tid.join();
  }
  ClearBuffers();
  close(fd_);
}
void WriteableCacheFile::ClearBuffers() {
  while (!bufs_.empty()) {
    auto it = bufs_.back();
#ifndef WITHOUT_LOG
    chLog(LOG_DEBUG, "[FlashCache] %s: dealloc buffers", Path().c_str());
#endif
    // if (!buf) {
    //   chLog(LOG_ERROR, "[FlashCache] already empty! Unable to dealloc
    //   buffers"); return false;
    // }
    bufs_.pop_back();
    delete it;
  }
}
int WriteableCacheFile::Delete() {
  Close();
  if (unlink(Path().c_str()) != 0) {
#ifndef WITHOUT_LOG
    chLog(LOG_ERROR, "[FlashCache] %s: Delete error %d %s", Path().c_str(),
          errno, strerror(errno));
#endif
    return 0;
  }
  return filesize_;
}
void WriteableCacheFile::Repair(void* metadata_) {
  size_t start_pos = 0;
  LBA lba_;
  lba_.cache_id_ = cache_id_;
  ssize_t r = -1;
  CacheRecordHeader hdr_;
  Metadata* pmetadata_ = reinterpret_cast<Metadata*>(metadata_);
  while (start_pos < filesize_) {
    r = pread(fd_, &hdr_, sizeof(CacheRecordHeader), start_pos);
    char* temp_key = new char[hdr_.key_size_];
    r = pread(fd_, temp_key, hdr_.key_size_, start_pos + sizeof(hdr_));
    std::string key_(temp_key, hdr_.key_size_);
    lba_.off_ = start_pos;
    lba_.size_ = hdr_.key_size_ + hdr_.val_size_ + sizeof(hdr_);
    pmetadata_->Insert(key_, lba_);
    start_pos += lba_.size_;
    delete[] temp_key;
  }
}
void WriteableCacheFile::Write() {
  sem_wait(sem_writer);
#ifndef WITHOUT_LOG
  chLog(LOG_NOTICE, "[FlashCache] (%d.rc): writerJobs start, %d/%d", cache_id_,
        buf_doff_, buf_woff_);
#endif
  while (buf_doff_ < buf_woff_) {
    auto buf_ = bufs_[buf_doff_];
    const char* src = buf_->Data();
    size_t left = buf_->Used();
    while (left != 0) {
      ssize_t done = write(fd_, src, left);
      if (done < 0) {
        if (errno == EINTR) {
          continue;
        }
        chLog(LOG_ERROR, "[FlashCache] %s: write error %d: %s", Path().c_str(),
              errno, strerror(errno));
        exit(-1);
      }
      left -= done;
      src += done;
    }
    filesize_ += buf_->Used();
    buf_doff_++;
  }
  is_writer_running = false;
}
void WriteableCacheFile::Write(const char* src, int size_) {
  size_t left = size_;
  while (left != 0) {
    ssize_t done = write(fd_, src, left);
    if (done < 0) {
      if (errno == EINTR) {
        continue;
      }
      chLog(LOG_ERROR, "[FlashCache] %s: write error %d: %s", Path().c_str(),
            errno, strerror(errno));
      exit(-1);
    }
    left -= done;
    src += done;
  }
  filesize_ += size_;
}
void* WriteableCacheFile::writerJobs(void* arg) {
  WriteableCacheFile* this_obj = (WriteableCacheFile*)arg;
#ifndef WITHOUT_LOG
  chLog(LOG_NOTICE, "[FlashCache] (%d.rc): writerJobs created",
        this_obj->cache_id_);
#endif
  for (;;) {
    this_obj->Write();
    if (this_obj->eof_) break;
  }
  this_obj->ClearBuffers();
#ifndef WITHOUT_LOG
  chLog(LOG_NOTICE, "[FlashCache] (%d.rc): writerJobs finished",
        this_obj->cache_id_);
#endif
  return NULL;
}

bool WriteableCacheFile::ExpandBuffer(const size_t size) {
  //   //   rwlock_.AssertHeld();

  // 현재 write buffer의 남은 공간 계산
  size_t free = 0;
  for (size_t i = buf_woff_; i < bufs_.size(); ++i) {
    free += bufs_[i]->Free();
    if (size <= free) {
      // 사용할 공간 < 남은 공간
      return true;
    }
  }
  // 버퍼 확장
  while (free < size) {
    CacheWriteBuffer* const buf = new CacheWriteBuffer(write_buffer_size_);
    if (!buf) {
      chLog(LOG_ERROR, "[FlashCache] Unable to allocate buffers");
      return false;
    }
    // size_ += static_cast<uint32_t>(buf->Free());
    free += buf->Free();
    bufs_.push_back(buf);
  }
  //   assert(free >= size);
  return true;
}
void WriteableCacheFile::DispatchBuffer() {
  // 현재 써지는 중이면 pass
  if (is_writer_running) return;
  // 파일이 꽉차지 않앗는데 아직 buffer의 한칸이 다 채워지지않았을땐 pass
  if (!eof_ && buf_doff_ == buf_woff_) return;
  // write thread가 마지막까지 모두 작업할 수 있도록 가짜 offset 증가.
  if (eof_) buf_woff_++;

  // write thread start
  sem_post(sem_writer);
  is_writer_running = true;
}
bool WriteableCacheFile::Append(const std::string& key, const std::string& val,
                                LBA* lba) {
  if (eof_) {
    return false;
  }
  // 파일에 쓸 데이터 크기 계산
  uint32_t rec_size = CacheRecord::CalcSize(key, val);
  lba->cache_id_ = cache_id_;
  lba->off_ = disk_woff_;
  lba->size_ = rec_size;
  CacheRecord rec(key, val);
  if (server.flashcache_mode == MODE_FLASH_WRITEBUFFER) {
    if (!ExpandBuffer(rec_size)) {
      // unable to expand the buffer
#ifndef WITHOUT_LOG
      chLog(LOG_ERROR, "[FlashCache] Error expanding buffers. size=%d",
            rec_size);
#endif
      return false;
    }
    // bufs에 serial된 rec를 기록
    if (!rec.Serialize(&bufs_, &buf_woff_)) {
      chLog(LOG_ERROR, "[FlashCache] Error serializing record");
      return false;
    }
    key_lists_.push_back(key);
    disk_woff_ += rec_size;
    eof_ = disk_woff_ >= max_size_;
    DispatchBuffer();
  } else {
    char* dest = new char[rec_size];
    memcpy(dest, reinterpret_cast<const char*>(&rec.hdr_), sizeof(rec.hdr_));
    memcpy(dest + sizeof(rec.hdr_), key.c_str(), key.size());
    memcpy(dest + sizeof(rec.hdr_) + key.size(), val.c_str(), val.size());
    Write(dest, rec_size);
    disk_woff_ += rec_size;
    if (filesize_ >= max_size_) eof_ = true;
    delete[] dest;
    key_lists_.push_back(key);
  }
  return true;
}

bool WriteableCacheFile::ReadBuffer(const LBA& lba, char* scratch) {
  char* tmp = scratch;
  size_t total_size = lba.size_;
  // write 버퍼는 write_buffer_size_에 맞춰서 여러개로 분할되어있으므로
  // 내가 원하는 데이터의 정확한 index를 계사해야 함.
  size_t start_idx = lba.off_ / write_buffer_size_;
  size_t start_off = lba.off_ % write_buffer_size_;

  assert(start_idx <= buf_woff_);

  for (size_t i = start_idx; total_size && i < bufs_.size(); ++i) {
    assert(i <= buf_woff_);
    auto* buf = bufs_[i];
    assert(i == buf_woff_ || !buf->Free());
    // bytes to write to the buffer
    // 읽어야할 전체 크기보다  bufs 한개 내에서 읽어와야할 크기가 작을땐,
    // 읽을 수 있는 양까지만 memcpy 하도록 nbytes 조정.
    size_t read_size = total_size > (buf->Used() - start_off)
                           ? (buf->Used() - start_off)
                           : total_size;
    memcpy(tmp, buf->Data() + start_off, read_size);

    // 나머지 데이터를 추가로 읽기 위해 포인터 주소와 offset 조정
    total_size -= read_size;
    start_off = 0;
    tmp += read_size;
  }
  assert(!total_size);
  if (total_size) {
    return false;
  }

  assert(tmp == scratch + lba.size_);
  return true;
}

bool WriteableCacheFile::Read(const LBA& lba, std::string* key,
                              std::string* val, char* scratch) {
  const bool closed = server.flashcache_mode == MODE_FLASH_WRITEBUFFER
                          ? eof_ && bufs_.empty()
                          : true;
  size_t read_size;
  if (!closed) {
    // 아직 파일을 쓰는 중이므로 버퍼에서 읽기
    if (!ReadBuffer(lba, scratch)) return false;
    read_size = lba.size_;
  } else {
    // 버퍼는 모두 지워졌으므로 파일에서만 읽기
    long offset = static_cast<long>(lba.off_);
    ssize_t r = -1;
    size_t left = lba.size_;
    char* data = scratch;
    while (left > 0) {
      r = pread(fd_, data, left, offset);
      if (r <= 0) {
        if (errno == EINTR) continue;
        break;
      }
      data += r;
      offset += r;
      left -= r;
    }
    read_size = (r < 0) ? 0 : lba.size_ - left;
  }
  CacheRecord rec;
  if (!rec.Deserialize(scratch, read_size)) {
#ifndef WITHOUT_LOG
    chLog(LOG_ERROR,
          "[FlashCache] Error de-serializing record from file %s off %d",
          Path().c_str(), lba.off_);
#endif
    return false;
  }
  *key = rec.key_;
  *val = rec.val_;
  return true;
}

// bool Metadata::Insert(const uint32_t cache_id, WriteableCacheFile* file) {
//   cache_file_index_.insert(std::make_pair(cache_id, file));
// #ifndef WITHOUT_LOG
//   chLog(LOG_DEBUG, "[FlashCache] insert.meta - key: %d, val: &file(%p)",
//         cache_id, static_cast<void*>(file));
// #endif
//   return true;
// }
bool Metadata::Insert(const uint32_t cache_id, WriteableCacheFile* file) {
  cache_file_index_.insert(cache_id, file);
#ifndef WITHOUT_LOG
  chLog(LOG_DEBUG, "[FlashCache] insert.meta - key: %d, val: &file(%p)",
        cache_id, static_cast<void*>(file));
#endif
  return true;
}
void Metadata::Insert(const std::string& key, const LBA& lba) {
  block_index_.insert(std::make_pair(key, lba));
#ifndef WITHOUT_LOG
  chLog(LOG_DEBUG,
        "[FlashCache] insert.meta - key: %s, val: (lba.cid: %d, lba.off: %d)",
        key.data(), lba.cache_id_, lba.off_);
#endif
}

bool Metadata::Lookup(const std::string& key, LBA* lba) {
  auto find_obj = block_index_.find(key);
  if (find_obj == block_index_.end()) {
    return false;
  }
  *lba = find_obj->second;
  return true;
}
// WriteableCacheFile* Metadata::Lookup(const uint32_t cache_id) {
//   auto find_obj = cache_file_index_.find(cache_id);
//   if (find_obj == cache_file_index_.end()) {
//     return nullptr;
//   }
//   return find_obj->second;
// }
WriteableCacheFile* Metadata::Lookup(const uint32_t cache_id) {
  auto find_obj = cache_file_index_.get(cache_id);
  return find_obj;
}
void Metadata::Erase(const std::string& key) { block_index_.erase(key); }
WriteableCacheFile* Metadata::Evict() {
  auto ret = cache_file_index_.evict();
  cache_file_index_.erase(ret->cacheid());
  return ret;
}
int FlashCache::Open() {
  std::vector<std::string> files;
  GetMultiFiles(opt_.path, &files);

  if (!files.empty()) {
    for (auto it : files) {
      LBA lba;
      if (it.size() > 3 &&
          it.substr(it.length() - 3, 3) == std::string(".rc")) {
        chLog(LOG_DEBUG, "[FlashCache] Repair MetaData! %s", it.c_str());
        int cache_id_ = atoi(it.substr(0, it.length() - 3).c_str());
        std::unique_ptr<WriteableCacheFile> f(
            new WriteableCacheFile(opt_.path, cache_id_, 0, 0));
        f->Open();
        f->Repair(&metadata_);
        metadata_.Insert(cache_id_, f.release());
        writer_cache_id_ =
            cache_id_ >= writer_cache_id_ ? cache_id_ + 1 : writer_cache_id_;
      }
    }
  }
  if (NewCacheFile()) {
#ifndef WITHOUT_LOG
    chLog(LOG_ERROR, "[FlashCache] Error creating new file %s.",
          opt_.path.c_str());
#endif
    return -1;
  }
  return 0;
}
int FlashCache::Close() {
  while (metadata_.cache_file_index_.size() > 0) {
    auto it = metadata_.cache_file_index_.evict();
    it->Close();
  }
  // for (auto it : metadata_.cache_file_index_) {
  //   it.second->Close();
  // }
  return 0;
}
// bool FlashCache::Erase(const Slice& key) override;

int FlashCache::NewCacheFile() {
  std::unique_ptr<WriteableCacheFile> f(
      new WriteableCacheFile(opt_.path, writer_cache_id_, opt_.cache_file_size,
                             opt_.write_buffer_size));
  if (!f->Create()) {
    chLog(LOG_ERROR, "[FlashCache] Error creating file");
    exit(-1);
  }
#ifndef WITHOUT_LOG
  chLog(LOG_NOTICE, "[FlashCache] (%d.rc): Created cache file",
        writer_cache_id_);
#endif
  cache_file_ = f.release();
  // Metadata insert for search
  if (!metadata_.Insert(writer_cache_id_, cache_file_)) {
    chLog(LOG_ERROR, "[FlashCache] Error inserting to metadata");
    exit(-1);
  }
  writer_cache_id_++;
  return 0;
}
int FlashCache::Insert(const std::string& key, const std::string& value) {
  LBA lba;
  while (!cache_file_->Append(key, value, &lba)) {
    if (!cache_file_->Eof()) {
#ifndef WITHOUT_LOG
      chLog(LOG_ERROR, "[FlashCache] Error inserting to cache file %d",
            cache_file_->cacheid());
#endif
      return -1;
    }
    if (NewCacheFile()) {
#ifndef WITHOUT_LOG
      chLog(LOG_ERROR, "[FlashCache] Error creating new file %s.",
            opt_.path.c_str());
#endif
      return -1;
    }
  }
  metadata_.Insert(key, lba);
  return 0;
}
char* FlashCache::Lookup(const std::string& key, size_t* size) {
  LBA lba;
  bool status;
  status = metadata_.Lookup(key, &lba);
  if (!status) {
    chLog(LOG_ERROR, "[FlashCache] lookup() key not found");
    return NULL;
  }
  WriteableCacheFile* file = metadata_.Lookup(lba.cache_id_);
  if (!file) {
    chLog(LOG_ERROR, "[FlashCache] error reading data");
    return NULL;
  }
  std::unique_ptr<char[]> scratch(new char[lba.size_]);
  std::string blk_key;
  std::string blk_val;

  status = file->Read(lba, &blk_key, &blk_val, scratch.get());
  char* val = new char[blk_val.size()];
  memcpy(val, blk_val.data(), blk_val.size());
  *size = blk_val.size();

  return val;
}
bool FlashCache::Evict() {
  WriteableCacheFile* f = metadata_.Evict();
  size_ -= f->Delete();
  for (auto it : f->key_lists()) {
    metadata_.Erase(it);
  }
  delete f;
  return true;
}

void FlashCache::DeleteMetaData(std::string& key) { metadata_.Erase(key); }