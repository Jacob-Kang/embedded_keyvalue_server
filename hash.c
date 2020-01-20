#include "hash.h"

static inline unsigned int get_hash_key(char *str) {
  unsigned int code = 5381, i;
  for (i = 0; i < str[i]; i++) code = (code * 33 + str[i]);
  return code;
}
int get_hash(struct hash *mc, char *str) {
  int table, idx;
  struct hashEntry *he;
  unsigned int hash_key = get_hash_key(str);
  //   for (table = 0; table <= 1; table++) {
  for (table = 0; table <= 0; table++) {
    idx = hash_key & mc->ht[table].sizemask;
    chLog(LOG_NOTICE, "[hash] hash_key: %d, idx: %d, msg: %s", hash_key, idx,
          str);
    he = mc->ht[table].table[idx];
    // 만약 해당 슬롯에 이미 다른 entry가 있을 경우 error return
    while (he) {
      if (!strcmp(he->key, str)) return -1;
      he = he->next;
    }
    // if (!dictIsRehashing(d)) break;
  }
  return idx;
}

int hashAdd(struct hash *mc, char *key, struct kvObject *val) {
  int index;
  struct hashEntry *entry;
  index = get_hash(mc, key);
  if (index == -1) return -1;
  entry = chmalloc(sizeof(*entry));
  entry->next = mc->ht[0].table[index];
  mc->ht[0].table[index] = entry;
  mc->ht[0].used++;

  entry->key = key;
  entry->val = val;
  chLog(LOG_DEBUG, "[hash] Add - hash_key: %d, idx: %d, msg: %s", key, index,
        val->ptr->buf);
  return 0;
}

struct hashEntry *entryFind(struct hash *mc, struct msg *key) {
  int table, idx;
  struct hashEntry *he;
  unsigned int hash_key = get_hash_key(key->buf);
  //   for (table = 0; table <= 1; table++) {
  for (table = 0; table <= 0; table++) {
    idx = hash_key & mc->ht[table].sizemask;
    he = mc->ht[table].table[idx];
    // 해당 슬롯에서 entry 찾기. 없으면 NULL
    while (he) {
      if (!strcmp(he->key, key->buf)) {
        chLog(LOG_DEBUG, "[hash] Find - hash_key: %d, idx: %d, msg: %s",
              hash_key, idx, he->key);
        return he;
      }
      he = he->next;
    }
    // if (!dictIsRehashing(d)) break;
  }
  return NULL;
}

int hashDelete(struct hash *mc, struct msg *key) {
  int idx;
  if (mc->ht[0].used > 0) {
    struct hashEntry *he, *prevHe;
    unsigned int hash_key = get_hash_key(key->buf);
    idx = hash_key & mc->ht[0].sizemask;
    he = mc->ht[0].table[idx];
    prevHe = NULL;
    while (he) {
      if (!strcmp(he->key, key->buf)) {
        chLog(LOG_NOTICE, "[hash] Delete - hash_key: %d, idx: %d, msg: %s",
              hash_key, idx, he->key);
        if (prevHe)
          prevHe->next = he->next;
        else
          mc->ht[0].table[idx] = he->next;
        chfree(he->key);
        chfree(he->val);
        chfree(he);
        mc->ht[0].used--;
        return 0;
      }
      prevHe = he;
      he = he->next;
    }
  }
  return -1;
}
