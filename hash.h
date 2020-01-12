#ifndef __HASH_H
#define __HASH_H
#include "server.h"
int hashAdd(struct hash *mc, char *key, struct kvObject *val);
struct hashEntry *entryFind(struct hash *mc, struct msg *key);

#endif