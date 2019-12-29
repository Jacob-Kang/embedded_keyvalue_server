#ifndef __UTIL_H
#define __UTIL_H

#include <errno.h>
#include <server.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
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
char *getAbsolutePath(char *filename);
void loadServerConfig(char *filename);
#endif
