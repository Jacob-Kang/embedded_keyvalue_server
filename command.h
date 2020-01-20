#ifndef __COMMAND_H
#define __COMMAND_H

#include "hash.h"
#include "server.h"
#include "util.h"
// #include "util_c++.h"
int memUsedCheck(struct kvClient *c);
int processCMD(struct kvClient *c);
void setCommand(struct kvClient *c, struct kvObject *key, struct kvObject *val);
int getCommand(struct kvClient *c, struct kvObject *key);
void evictAllCommand();
#endif