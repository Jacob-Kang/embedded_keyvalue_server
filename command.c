#include "command.h"

void dbAdd(struct kvDb *db, struct kvObject *key, struct kvObject *val) {
  char *copy = msgdup((struct msg *)key->ptr);
  int retval = hashAdd(db->memCache, copy, val);
  if (retval == -1) chLog(LOG_ERROR, "dbAdd error");
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

int memUsedCheck() {
  size_t mem_used;
  mem_used = get_used_memory();

  // flush 시킬 object background에게 보내기
  //   if (mem_used > server.maxmemory * 90 / 100) {
  //     int j, k;
  //     for (j = 0; j < 1; j++) {
  //       char *bestkey = NULL;
  //       struct hashEntry *de;
  //       struct kvDb *db = server.db + j;
  //       struct hash *dict;
  //       struct kvObject *valueObj;
  //       dict = server.db[j].expires;
  //       if (!isFull(db->populateQueue)) {
  //         struct evictionPoolEntry *pool = db->eviction_pool;
  //         int retry_cnt = 0;
  //         while (bestkey == NULL && retry_cnt < 10) {
  //           evictionPoolPopulate(dict, db->dict, db->eviction_pool);
  //           /* Go backward from best to worst element to evict. */
  //           for (k = REDIS_EVICTION_POOL_SIZE - 1; k >= 0; k--) {
  //             if (pool[k].key == NULL) continue;
  //             de = dictFind(dict, pool[k].key);

  //             /* Remove the entry from the pool. */
  //             sdsfree(pool[k].key);
  //             /* Shift all elements on its right to left. */
  //             memmove(pool + k, pool + k + 1,
  //                     sizeof(pool[0]) * (REDIS_EVICTION_POOL_SIZE - k - 1));
  //             /* Clear the element on the right which is empty
  //              * since we shifted one position to the left.  */
  //             pool[REDIS_EVICTION_POOL_SIZE - 1].key = NULL;
  //             pool[REDIS_EVICTION_POOL_SIZE - 1].idle = 0;

  //             /* If the key exists, is our pick. Otherwise it is
  //              * a ghost and we need to try the next element. */
  //             if (de) {
  //               valueObj = dictGetVal(de);
  //               // CHKang: without entry already enqueued
  //               if (valueObj->where == REDIS_LOC_REDIS) {
  //                 bestkey = dictGetKey(de);
  //                 // Because of inclusive cache policy, this key is already
  //                 exist. if (valueObj->writecount == 0) {
  //                   redisLog(REDIS_WARNING,
  //                            "[DELETE] %s\tjust delete cause of clean data.",
  //                            bestkey);
  //                   valueObj->where = REDIS_LOC_NVM;
  //                 } else {
  //                   flushRedisDictToNVM(db, de, false);
  //                 }
  //                 // redisLog(REDIS_WARNING, "Mem used: %f%, ",
  //                 // (float)mem_used/server.maxmemory*100);
  //                 int status;
  //                 if ((status = enqueue(&(db->populateQueue), de)) ==
  //                     ENQUEUE_FAILURE) {
  //                   redisLog(REDIS_WARNING,
  //                            "Must be evicted before nvwriteCommand.");
  //                   redisAssert(0);
  //                 }
  //                 break;
  //               }
  //             } else {
  //               /* Ghost... */
  //               continue;
  //             }
  //           }
  //           retry_cnt++;
  //         }
  //       }
  //     }
  //   }
  //   mem_used = getUsedMemory();
  //   /* Check if we are over the memory limit. */
  //   if (mem_used < server.maxmemory) return 0;
  //   /* Used memory exceeds threshold. */
  //   if (mem_used >= server.maxmemory) {
  //     struct hashEntry *target = NULL;
  //     int j, k, keys_freed = 0;
  //     for (j = 0; j < 1; j++) {
  //       struct kvDb *db = server.db + j;
  //       char *bestkey = NULL;
  //       struct kvObject *valueObj;
  //     force_flush:
  //       while (keys_freed < 1 && (target = dequeue(db->populateQueue)) !=
  //       NULL) {
  //         bestkey = dictGetKey(target);
  //         // robj *valobj = dictGetVal(target);
  //         struct kvObject *keyobj = createStringObject(bestkey,
  //         sdslen(bestkey));
  //         // redisLog(REDIS_WARNING, "Mem used: %f%\t[PQ]Dequeue(%d/%d) WR:
  //         %d,
  //         // \t%x\t%s", (float)mem_used/server.maxmemory*100,
  //         // PQsize(db->priorityQueue), PQMaxsize(db->priorityQueue),
  //         // valobj->writecount, target, bestkey); redisLog(REDIS_WARNING,
  //         "Mem
  //         // used: %f%\t[PQ]Dequeue(%d/%d) WR: %d, \t%x\t%s",
  //         // (float)mem_used/server.maxmemory*100, PQsize(db->priorityQueue),
  //         // PQMaxsize(db->priorityQueue), valobj->writecount, target,
  //         bestkey);
  //         // propagateExpire(db, keyobj);
  //         struct kvObject *argv[2];
  //         // argv[0] = shared.del;
  //         argv[1] = keyobj;
  //         dbDelete(db, keyobj);
  //         keys_freed++;
  //         // dbEvict(db, keyobj);
  //         // db->evictedKeys++;
  //         // server.stat_evictedkeys++;
  //       }
  //       mem_used = getUsedMemory();
  //     }
  //     if (!keys_freed) {
  //       if (mem_used > server.maxmemory * 110 / 100) {
  //         chLog(
  //             LOG_NOTICE,
  //             "Memory is full. Eviction does not guarantee the capable
  //             memory.");
  //         return -1; /* nothing to free... */
  //       }
  //       if (mem_used > server.maxmemory * 120 / 100) goto force_flush;
  //     }
  //   }
  //   return 0;
  // }
  void setGenericCommand(struct kvClient * c, struct kvObject * key,
                         struct kvObject * val) {
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

  int getGenericCommand(struct kvClient * c, struct kvObject * key) {
    struct hashEntry *he = lookupKey(c->db, key);
    if (he) {
      c->querybuf->len = sprintf(c->querybuf->buf, "$%d\r\n%s\r\n",
                                 he->val->ptr->len, he->val->ptr->buf);
      return 0;
    } else
      c->querybuf->len = sprintf(c->querybuf->buf, "$7\r\n+Empty!\r\n");
    return NULL;
  }