#ifndef __COMMAND_H
#define __COMMAND_H

#include "hash.h"
#include "server.h"
#include "util.h"
// #include "util_c++.h"
int memUsedCheck(struct kvClient *c);
int processCMD(struct kvClient *c);
void setGenericCommand(struct kvClient *c, struct kvObject *key,
                       struct kvObject *val);
int getGenericCommand(struct kvClient *c, struct kvObject *key);

#endif