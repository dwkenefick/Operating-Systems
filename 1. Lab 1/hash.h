/*Implementation of a hashtable in c, by Dan Kenefick*/

#ifndef HASH_H
#define HASH_H
typedef struct hashTable *hash;

typedef struct hashTable
{
  int size;
  struct cirq * buckets;

} hashTable;

typedef struct pair {
	char *key;
	char *value;
} pair;

extern hash ht_alloc(int buckets);
extern void ht_put(hash h, char *key, void *value);
extern void *ht_get(hash h, char *key);
extern void ht_free(hash h);

unsigned long ht_hash(char *str);

#endif
