#ifndef __UTIL_H
#define __UTIL_H

#include <errno.h>
#include <limits.h>
#include <server.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "server.h"

struct sdshdr {
  unsigned int len;
  unsigned int free;
  char buf[];
};
// LOG_DEBUG 0
// LOG_NOTICE 1
// LOG_ERROR 2
void chkangLog(int level, const char *fmt, ...);
char *sdsnewlen(const void *init, size_t initlen);
char *sdsnew(const char *init);
void sdsfree(char *s);
int string2ll(const char *s, size_t slen, long long *value);
char *getAbsolutePath(char *filename);
void loadServerConfig(char *filename);
void parsingMessage(struct kvClient *c);
struct kvObject *createObject(void *ptr, size_t len);
#endif
