#ifndef __SERVER_H
#define __SERVER_H
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/syscall.h>
#include "bworker.h"
#include "net.h"
#include "util.h"
/* Log levels */
#define LOG_DEBUG 0
#define LOG_NOTICE 1
#define LOG_ERROR 2

#define SERVERPORT 6379 /* TCP port */
#define IP_STR_LEN 46
#define MAX_LOGMSG_LEN 1024

#define MAX_NUM_CLIENT 10

#define KV_IOBUF_LEN (1024 * 16) /* Generic I/O buffer size */

#define LRU_BITS 22

#define KV_LOC_MEM 0
#define KV_LOC_FLUSHED 1
#define KV_LOC_SD 2
typedef struct kvObject {
  volatile unsigned
      where : 2;  // specifies where the k/v is located; 0: REDIS_LOC_REDIS, 1:
                  // REDIS_LOC_FORCEFLUSHED, 2: REDIS_LOC_FLASH
  unsigned type : 3;  // we borrow one bit from this; the current number of
                      // redis types is 5
  // unsigned encoding : 4;
  // unsigned shared : 1;
  unsigned lru : LRU_BITS;  // lru time (relative to server.lruclock)
  int refcount;
  uint32_t readcount;
  uint32_t writecount;
  void *ptr;
} kvobj;

typedef struct dictEntry {
  void *key;
  union {
    void *val;
    long long s64;
  } v;
  struct dictEntry *next;
} dictEntry;

typedef struct dictType {
  unsigned int (*hashFunction)(const void *key);
  void *(*keyDup)(void *privdata, const void *key);
  void *(*valDup)(void *privdata, const void *obj);
  int (*keyCompare)(void *privdata, const void *key1, const void *key2);
  void (*keyDestructor)(void *privdata, void *key);
  void (*valDestructor)(void *privdata, void *obj);
} dictType;

/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
typedef struct dictht {
  dictEntry **table;
  unsigned long size;
  unsigned long sizemask;
  unsigned long used;
} dictht;

typedef struct dict {
  dictType *type;
  void *privdata;
  dictht ht[2];
  long rehashidx; /* rehashing not in progress if rehashidx == -1 */
  int iterators;  /* number of iterators currently running */
} dict;

// typedef struct kvDb {};

struct msg {
  unsigned int len;
  // unsigned int free;
  char buf[];
};

struct kvClient {
  uint64_t id;
  int fd;
  struct kvDb *db;
  int argc;
  struct kvObject **argv;
  size_t sentlen;
  struct msg *querybuf;
};

struct kvServer {
  int pid; /* Main process pid. */
  char *configfile;
  struct kvDb *db;
  struct kvClient *clients[MAX_NUM_CLIENT];
  int num_connected_client;
  /* Networking */
  int clnt_sock;
  int port;
  int ipfd;
  int cli_fd;
  // list *clients;
  // db
  char *db_dir;
  uint64_t db_size;
  uint32_t db_file_size;

  int verbosity;
  char *logfile;
};

extern struct kvServer server;

#endif