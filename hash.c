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
    chkangLog(LOG_NOTICE, "hash_key: %d, idx: %d\nmsg: %s", hash_key, idx, str);
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
  // struct kvht *ht;

  //   if (dictIsRehashing(d)) _dictRehashStep(d);

  /* Get the index of the new element, or -1 if
   * the element already exists. */
  index = get_hash(mc, key);
  if (index == -1) return -1;

  /* Allocate the memory and store the new entry */
  //   ht = dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];
  entry = malloc(sizeof(*entry));
  entry->next = mc->ht[0].table[index];
  mc->ht[0].table[index] = entry;
  mc->ht[0].used++;

  entry->key = key;
  entry->val = val;
  return 0;
}

struct hashEntry *entryFind(struct hash *mc, struct msg *key) {
  int table, idx;
  struct hashEntry *he;
  // if (mc->ht[0].size == 0) return NULL; /* We don't have a table at all */
  // if (dictIsRehashing(d)) _dictRehashStep(d);
  unsigned int hash_key = get_hash_key(key->buf);
  //   for (table = 0; table <= 1; table++) {
  for (table = 0; table <= 0; table++) {
    idx = hash_key & mc->ht[table].sizemask;
    he = mc->ht[table].table[idx];
    // 해당 슬롯에서 entry 찾기. 없으면 NULL
    while (he) {
      if (!strcmp(he->key, key->buf)) {
        chkangLog(LOG_NOTICE, "hash_key: %d, idx: %d\nmsg: %s", hash_key, idx,
                  he->key);
        return he;
      }
      he = he->next;
    }
    // if (!dictIsRehashing(d)) break;
  }
  return NULL;
}
