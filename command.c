#include "command.h"

void dbAdd(struct kvDb *db, struct kvObject *key, struct kvObject *val) {
  char *copy = msgdup((struct msg *)key->ptr);
  int retval = hashAdd(db->memCache, copy, val);
  if (retval == -1) chkangLog(LOG_ERROR, "dbAdd error");
  // redisAssertWithInfo(NULL, key, retval == REDIS_OK);
  // if (val->type == REDIS_LIST) signalListAsReady(db, key);
}

void dbOverwrite(struct hashEntry *he, struct kvObject *val) {
  //   dictEntry *de = dictFind(db->dict, key->ptr);
  he->val = val;
  //   redisAssertWithInfo(NULL, key, de != NULL);
  //   dictReplace(db->dict, key->ptr, val);
}

struct hashEntry *lookupKey(struct kvDb *db, struct kvObject *key) {
  struct hashEntry *he = entryFind(db->memCache, key->ptr);
  if (he) {
    // struct msg *val = he->val;

    /* Update the access time for the ageing algorithm.
     * Don't do it if we have a saving child, as this will trigger
     * a copy on write madness. */
    // val->lru = LRU_CLOCK();
    // return val;
    return he;
  } else {
    return NULL;
  }
}

int processCMD(struct kvClient *c) {
  if (msgcmp(c->argv[0]->ptr, "set")) {
    memUsedCheck();
    setGenericCommand(c, c->argv[1], c->argv[2]);
  } else if (msgcmp(c->argv[0]->ptr, "get")) {
    getGenericCommand(c, c->argv[1]);
  } else if (msgcmp(c->argv[0]->ptr, "scan")) {
  }
}

int memUsedCheck() {}
void setGenericCommand(struct kvClient *c, struct kvObject *key,
                       struct kvObject *val) {
  struct hashEntry *he = lookupKey(c->db, key);
  if (he == NULL) {
    // if (lookupKeyWrite(c->db, key) == NULL) {
    // incrCount(val, WRITECOUNT);
    dbAdd(c->db, key, val);
    // server.stat_write_misses++;
  } else {
    // incrCount(val, WRITECOUNT);
    // val->where = REDIS_LOC_UPDATED;
    dbOverwrite(he, val);
    // server.stat_write_hits++;
  }
  char *msg = "+OK\r\n";
  c->querybuf->len = strlen(msg);
  memcpy(c->querybuf->buf, msg, c->querybuf->len);
}

int getGenericCommand(struct kvClient *c, struct kvObject *key) {
  struct hashEntry *he = lookupKey(c->db, key);
  if (he) {
    c->querybuf->len = sprintf(c->querybuf->buf, "$%d\r\n%s\r\n",
                               he->val->ptr->len, he->val->ptr->buf);
    // memcpy(c->querybuf->buf, he->val->ptr->buf, he->val->ptr->len);
    return 0;
  } else
    c->querybuf->len = sprintf(c->querybuf->buf, "$7\r\n+Empty!\r\n");
  return NULL;
}