#ifndef __COMMAND_H
#define __COMMAND_H

#include "hash.h"
#include "server.h"
#include "util.h"
int processCMD(struct kvClient *c);
int memUsedCheck();
void setGenericCommand(struct kvClient *c, struct kvObject *key,
                       struct kvObject *val);
int getGenericCommand(struct kvClient *c, struct kvObject *key);

#endif