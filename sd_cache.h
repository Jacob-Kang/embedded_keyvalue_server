struct flashCacheConfig {
  explicit PersistentCacheConfig(
      Env* const _env, const std::string& _path, const uint64_t _cache_size,
      const std::shared_ptr<Logger>& _log,
      const uint32_t _write_buffer_size = 1 * 1024 * 1024 /*1MB*/) {
    env = _env;
    path = _path;
    log = _log;
    cache_size = _cache_size;
    writer_dispatch_size = write_buffer_size = _write_buffer_size;
  }
}