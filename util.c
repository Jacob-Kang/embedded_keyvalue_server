#include "util.h"
void chkangLog(int level, const char *fmt, ...) {
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
    sh = malloc(sizeof(struct sdshdr) + initlen + 1);
  } else {
    sh = calloc(1, sizeof(struct sdshdr) + initlen + 1);
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
        char *p = malloc(l);
        memcpy(p, argv[1], l);
        server.db_dir = p;
      } else if (!strcasecmp(argv[0], "db-size-mb") && argc == 2) {
        server.db_size = atoi(argv[1]) * 1024L * 1024;
        if (server.db_size <= 0) {
          printf("Invalid db-size-mb, value <= 0 is not allowed");
          exit(-1);
        }
      } else if (!strcasecmp(argv[0], "db-file-size-mb") && argc == 2) {
        server.db_file_size = atoi(argv[1]) * 1024L * 1024;
        if (server.db_size <= 0) {
          printf("Invalid db-file-size-mb, value <= 0 is not allowed");
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
        server.logfile = malloc(l);
        argv[1][l - 1] = 0;
        memcpy(server.logfile, argv[1], l);
        if (server.logfile[0] != '\0') {
          /* Test if we are able to open the file. The server will not
           * be able to abort just for this problem later... */
          logfp = fopen(server.logfile, "a");
          if (logfp == NULL) {
            server.logfile[0] = '\0';
            chkangLog(LOG_NOTICE, "Can't open the log file: %s",
                      strerror(errno));
            exit(-1);
          }
          fclose(logfp);
        }
      }
      argc = 0;
    }
    if (fp != stdin) fclose(fp);
  }
}

void parsingMessage(struct kvClient *c) {
  if (c->querybuf->buf[0] != '*') {
    chkangLog(LOG_ERROR, "Unknown request type");
    return;
  }
  char *newline = NULL;
  newline = strchr(c->querybuf->buf, '\r');
  if (newline == NULL) {
    chkangLog(LOG_ERROR, "Protocol error: unknown msg_len");
    return;
  }
  int msg_len = c->querybuf->buf[1] - '0';
  // split 한 시점부터 \r\n만큼 뛴 위치를 새로운 pos로 저장
  int pos = (newline - c->querybuf->buf) + 2;
  long long len = 0;
  c->argc = 0;
  c->argv = malloc(sizeof(struct kvObject *) * msg_len);
  while (msg_len) {
    newline = strchr(c->querybuf->buf + pos, '\r');
    if (c->querybuf->buf[pos] != '$') {
      chkangLog(LOG_ERROR, "Protocol error: Unknow cmd len");
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

kvobj *createObject(void *ptr, size_t len) {
  kvobj *o = malloc(sizeof(kvobj) + sizeof(struct msg) + len + 1);
  struct msg *msg = (void *)(o + 1);

  o->where = KV_LOC_MEM;
#ifdef ONLY_SD
  o->where = KV_LOC_SD; /* key/value is located only in REDIS */
#endif
  o->ptr = ptr;
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