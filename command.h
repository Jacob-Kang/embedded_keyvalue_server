#ifndef __COMMAND_H
#define __COMMAND_H

#include "server.h"

int processCMD(struct kvClient *c);
int memUsedCheck();
void setGenericCommand(struct kvClient *c, int flags, struct kvObject *key,
                       struct kvObject *val);

#endif