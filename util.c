#include "util.h"
void chLog(int level, const char *fmt, ...) {
  va_list ap;
  if ((level & 0xff) < server.verbosity) return;

  char msg[MAX_LOGMSG_LEN];
  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);

  FILE *fp;
  const char *c = "-*#";
  char buf[64];
  int off;
  struct timeval tv;
  int role_char;
  int log_to_stdout = server.logfile == '\0';
  fp = log_to_stdout ? stdout : fopen(server.logfile, "a");

  gettimeofday(&tv, NULL);
  off = strftime(buf, sizeof(buf), "%d %b %H:%M:%S.", localtime(&tv.tv_sec));
  snprintf(buf + off, sizeof(buf) - off, "%03d", (int)tv.tv_usec / 1000);
  fprintf(fp, "[P:%d %s] %c %s\n", (int)getpid(), buf, c[level], msg);
  fflush(fp);
  if (!log_to_stdout) fclose(fp);
}

char *sdsnewlen(const void *init, size_t initlen) {
  struct sdshdr *sh;

  if (init) {
    sh = chmalloc(sizeof(struct sdshdr) + initlen + 1);
  } else {
    sh = chcalloc(sizeof(struct sdshdr) + initlen + 1);
  }
  if (sh == NULL) return NULL;
  sh->len = initlen;
  sh->free = 0;
  if (initlen && init) memcpy(sh->buf, init, initlen);
  sh->buf[initlen] = '\0';
  return (char *)sh->buf;
}

char *sdsnew(const char *init) {
  int initlen = (init == NULL) ? 0 : strlen(init);
  return sdsnewlen(init, initlen);
}
void sdsfree(char *s) {
  if (s == NULL) return;
  free(s - sizeof(struct sdshdr));
}
int msgcmp(const struct msg *s, const char *dest) {
  char *src = (char *)(s->buf);
  if (!strcasecmp(src, dest))
    return 1;
  else
    return 0;
}
char *msgdup(const struct msg *s) {
  char *ret = chmalloc(s->len + 1);
  if (s == NULL) return NULL;
  memcpy(ret, s->buf, s->len);
  ret[s->len] = '\0';
  return ret;
}

int string2ll(const char *s, size_t slen, long long *value) {
  const char *p = s;
  size_t plen = 0;
  int negative = 0;
  unsigned long long v;
  if (plen == slen) return 0;
  if (p[0] >= '1' && p[0] <= '9') {
    v = p[0] - '0';
    p++;
    plen++;
  } else if (p[0] == '0' && slen == 1) {
    *value = 0;
    return 1;
  } else {
    return 0;
  }
  while (plen < slen && p[0] >= '0' && p[0] <= '9') {
    if (v > (ULLONG_MAX / 10)) /* Overflow. */
      return 0;
    v *= 10;
    if (v > (ULLONG_MAX - (p[0] - '0'))) /* Overflow. */
      return 0;
    v += p[0] - '0';
    p++;
    plen++;
  }
  *value = v;
  return 1;
}
char *getAbsolutePath(char *filename) {
  char cwd[1024];

  char *abspath;
  char *relpath = sdsnew(filename);

  // 시작 주소가  root이면 절대주소로 간주
  if (relpath[0] == '/') return relpath;
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    sdsfree(relpath);
    return NULL;
  }
  abspath = sdsnew(cwd);
  abspath = strcat(abspath, "/");
  abspath = strcat(abspath, relpath);
  sdsfree(relpath);
  return abspath;
}

void loadServerConfig(char *filename) {
  /* Load the file content */
  char buf[1024];
  char *argv[10];
  int argc = 0;
  if (filename) {
    FILE *fp;
    if ((fp = fopen(filename, "r")) == NULL) {
      printf("Fatal error, can't open config file '%s'", filename);
      exit(1);
    }
    while (fgets(buf, 1024, fp) != NULL) {
      if (buf[0] == '#' || buf[0] == '\0' || buf[0] == '\n') continue;
      argv[argc] = strtok(buf, " ");
      while (argv[argc] != NULL) {
        argv[++argc] = strtok(NULL, " ");  // 다음 문자열을 잘라서 포인터를 반환
      }
      if (!strcasecmp(argv[0], "port") && argc == 2) {
        server.port = atoi(argv[1]);
      } else if (!strcasecmp(argv[0], "db-dir") && argc == 2) {
        int l = strlen(argv[1]);
        argv[1][l - 1] = 0;
        char *p = chmalloc(l);
        memcpy(p, argv[1], l);
        server.db_dir = p;
      } else if (!strcasecmp(argv[0], "memcache-size-mb") && argc == 2) {
        server.maxmemory = atoi(argv[1]) * 1024L * 1024;
        if (server.maxmemory <= 0) {
          chLog(LOG_ERROR, "Invalid db-size-mb, value <= 0 is not allowed");
          exit(-1);
        }
      } else if (!strcasecmp(argv[0], "flashcache-dir") && argc == 2) {
        zfree(server.flashCache_dir);
        server.flashCache_dir = zstrdup(argv[1]);
      } else if (!strcasecmp(argv[0], "flashcache-size-mb") && argc == 2) {
        server.flashCache_size = atoi(argv[1]) * 1024L * 1024;
        if (server.flashCache_size <= 0) {
          chLog(LOG_ERROR,
                "Invalid nvmcache-size-mb, value <= 0 is not allowed");
          exit(-1);
        }
      } else if (!strcasecmp(argv[0], "flashcache-file-size-kb") && argc == 2) {
        server.flashCache_file_size = atoi(argv[1]) * 1024;
        if (server.flashCache_file_size <= 0) {
          chLog(LOG_ERROR,
                "Invalid nvmcache-file-size-kb, value <= 0 is not allowed");
          exit(-1);
        }
      } else if (!strcasecmp(argv[0], "flashcache-file-size-mb") && argc == 2) {
        server.flashCache_file_size = atoi(argv[1]) * 1024 * 1024;
        if (server.flashCache_file_size <= 0) {
          chLog(LOG_ERROR,
                "Invalid nvmcache-file-size-mb, value <= 0 is not allowed");
          exit(-1);
        }
      } else if (!strcasecmp(argv[0], "log-level") && argc == 2) {
        if (!strcasecmp(argv[1], "notice")) {
          server.verbosity = LOG_NOTICE;
        } else if (!strcasecmp(argv[1], "debug")) {
          server.verbosity = LOG_DEBUG;
        }
      } else if (!strcasecmp(argv[0], "logfile") && argc == 2) {
        FILE *logfp;
        free(server.logfile);
        size_t l = strlen(argv[1]);
        server.logfile = chmalloc(l);
        argv[1][l - 1] = 0;
        memcpy(server.logfile, argv[1], l);
        if (server.logfile[0] != '\0') {
          /* Test if we are able to open the file. The server will not
           * be able to abort just for this problem later... */
          logfp = fopen(server.logfile, "a");
          if (logfp == NULL) {
            server.logfile[0] = '\0';
            chLog(LOG_NOTICE, "Can't open the log file: %s", strerror(errno));
            exit(-1);
          }
          fclose(logfp);
        }
      } else if (!strcasecmp(argv[0], "server-dir") && argc == 2) {
        if (chdir(argv[1]) == -1) {
          chLog(LOG_NOTICE, "Can't chdir to '%s': %s", argv[1],
                strerror(errno));
          exit(1);
        }
      }
      argc = 0;
    }
    if (fp != stdin) fclose(fp);
  }
}

void parsingMessage(struct kvClient *c) {
  if (c->querybuf->buf[0] != '*') {
    chLog(LOG_ERROR, "Unknown request type");
    return;
  }
  char *newline = NULL;
  newline = strchr(c->querybuf->buf, '\r');
  if (newline == NULL) {
    chLog(LOG_ERROR, "Protocol error: unknown msg_len");
    return;
  }
  int msg_len = c->querybuf->buf[1] - '0';
  // split 한 시점부터 \r\n만큼 뛴 위치를 새로운 pos로 저장
  int pos = (newline - c->querybuf->buf) + 2;
  long long len = 0;
  c->argc = 0;
  if (c->argv) {
    int i;
    for (i = 0; i < 2; i++) chfree(c->argv[i]);
  }
  c->argv = chmalloc(sizeof(struct kvObject *) * msg_len);
  while (msg_len) {
    newline = strchr(c->querybuf->buf + pos, '\r');
    if (c->querybuf->buf[pos] != '$') {
      chLog(LOG_ERROR, "Protocol error: Unknow cmd len");
      return;
    }
    string2ll(c->querybuf->buf + pos + 1,
              newline - (c->querybuf->buf + pos) + 1, &len);
    // len = c->querybuf->buf[pos + 1] - '0';
    pos += newline - (c->querybuf->buf + pos) + 2;
    c->querybuf->buf[pos + len] = 0;
    c->argv[c->argc++] = createObject(c->querybuf->buf + pos, len);
    pos += len + 2;
    c->querybuf->len = len;
    msg_len--;
  }
}

struct kvObject *createObject(void *ptr, size_t len) {
  struct kvObject *o =
      chmalloc(sizeof(struct kvObject) + sizeof(struct msg) + len + 1);
  struct msg *msg = o + 1;

  o->where = KV_LOC_MEM;
#ifdef ONLY_SD
  o->where = KV_LOC_SD; /* key/value is located only in REDIS */
#endif
  o->ptr = msg;
  o->refcount = 1;
  // o->lru = LRU_CLOCK();
  o->writecount = 0;
  o->readcount = 0;
  msg->len = len;
  // msg->free = 0;
  if (ptr) {
    memcpy(msg->buf, ptr, len);
    msg->buf[len] = '\0';
  } else {
    memset(msg->buf, 0, len + 1);
  }
  return o;
}

static int used_memory;
pthread_mutex_t memory_mutex = PTHREAD_MUTEX_INITIALIZER;
void *chmalloc(size_t size) {
  void *ptr = malloc(size);
  pthread_mutex_lock(&memory_mutex);
  used_memory += size;
  pthread_mutex_unlock(&memory_mutex);
  return ptr;
}
void *chcalloc(size_t size) {
  void *ptr = calloc(1, size);
  pthread_mutex_lock(&memory_mutex);
  used_memory += size;
  pthread_mutex_unlock(&memory_mutex);

  return ptr;
}
void chfree(void *ptr) {
  pthread_mutex_lock(&memory_mutex);
  used_memory -= malloc_size(ptr);
  pthread_mutex_unlock(&memory_mutex);
  free(ptr);
}
size_t get_used_memory(void) {
  size_t um;
  pthread_mutex_lock(&memory_mutex);
  um = used_memory;
  pthread_mutex_unlock(&memory_mutex);
  return um;
}