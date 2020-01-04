#include "command.h"

int processCMD(struct kvClient *c) {
  if (!strcasecmp(c->argv[0]->ptr, "set")) {
    memUsedCheck();
    // setGenericCommand();
  } else if (!strcasecmp(c->argv[0]->ptr, "get")) {
  } else if (!strcasecmp(c->argv[0]->ptr, "scan")) {
  }
}

int memUsedCheck() {}
void setGenericCommand(struct kvClient *c, int flags, struct kvObject *key,
                       struct kvObject *val) {}
