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

typedef struct kvDb {};

struct kvServer {
  int pid; /* Main process pid. */
  char *configfile;
  struct kvDb *db;
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