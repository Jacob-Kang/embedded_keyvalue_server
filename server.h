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

// typedef struct lookupResult {
//   struct kvObject *valueObj;
//   uint64_t isInFlashDb : 1,
//       rowCount : 63;  // due to the memory alignment in x64 this is ok
//   int *filteredRowIds;
// }

struct kvObject {
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
  struct msg *ptr;
};

struct hashEntry {
  char *key;
  struct kvObject *val;
  // union {
  //   void *val;
  //   long long s64;
  // } v;
  struct hashEntry *next;
};

/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
struct kvht {
  struct hashEntry **table;
  unsigned long size;
  unsigned long sizemask;
  unsigned long used;
};

struct hash {
  // dictType *type;
  // void *privdata;
  struct kvht ht[2];
  long rehashidx; /* rehashing not in progress if rehashidx == -1 */
  int iterators;  /* number of iterators currently running */
};

struct kvDb {
  struct hash *memCache;
  // Queue *populateQueue;
  void *sdCache;
};

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