#ifndef __UTIL_H
#define __UTIL_H

#include <stdio.h>
#include "client.h"

// LOG_DEBUG 0
// LOG_NOTICE 1
// LOG_ERROR 2

int string2ll(const char *s, size_t slen, long long *value);
void loadServerConfig(char *filename);
void makingMessage(struct kvClient *c);

#endif