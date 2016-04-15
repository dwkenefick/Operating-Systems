/*implementation for a hash table in C, by Dan Kenefick*/
#ifndef HASH_C
#define HASH_C

#include "hash.h"
#include "cirq.h"
#include <stdlib.h>
#include <string.h>

/*
//test code
int main(){
  int data [5] = {1,2,3,4,5};
  int *d = &data[0];
  int *temp;
  hash h;

  h = ht_alloc(1);

  ht_put(h,"first",d);
  d++;
  ht_put(h,"second",d);
  d++;
  ht_put(h,"third",d);
  d++;
  ht_put(h,"fourth",d);

  temp = ht_get(h,"first");
  printf("the first value is: %d",*temp);

  temp = ht_get(h,"second");

  printf("the second value is: %d",*temp);

  return 0;
}

*/

/*
allocs a new hash table
creates a new hash table, and initializes the array of circqs to be used for the buckets

the number of buckets must be posititve
 */

extern hash ht_alloc(int buckets){

  hashTable *htable;
  int x;
  cirq * temp;

  if (buckets <=0) return NULL;

  htable = malloc(sizeof(hashTable));

  /* makes sure that the malloc goes ok*/
  if (htable == NULL) {
      	return NULL;
  }

  htable->size = buckets;

  /* allocates memory for each of the buckets*/
  htable->buckets = malloc( buckets * sizeof( cirq ) );

  /* makes sur ethat the malloc goes ok*/
   if (htable->buckets == NULL) {
    	free(htable);
       	return NULL;
    }

   temp = (cirq *)htable->buckets;

   for(x=0;x<buckets;x++){
     temp[x] = cq_alloc();
     }
  return htable;

}


/*
Frees all memory ascociated with the hashtable.
 */
extern void ht_free(hash h){
  int n,i,size,j;
  cirq *q;
  pair *p;

  if (!h) return;
  

  q = (cirq *)h->buckets;
  n = h->size;
  i = 0;

  /*frees each of the queues in the hashtable, and its included data*/
  while ( i < n){
    size = cq_size(*q);
    for(j = 0; j< size; j++){
      /* We cannot use cq_free here, it would jsut free the pair structure, not
	 the key and value it points to. So we have to walk through each queue, 
	 freeing the items within each pair
      */
      p = (pair*) cq_deq(*q);
      free(p->key);
      free(p->value);
      free(p);
    }
    i++;
    q++;
  }

  free(h -> buckets);
  free(h);

  /*make sure the table is not pointing to anything nasty later*/
  h = NULL;

  return;
}


/*
puts a value in the hash table
 */
extern void ht_put(hash h, char *key, void *value){

  unsigned long index;
  int key_len;
  cirq q;
  cirq * buckets;
  pair *p;
  char *new_key;
  void *new_value;

  int i, size,comp;

  if (!h  || !key ) return;

  key_len = strlen(key);
  index = ht_hash (key)%(h->size);

  buckets = (cirq *)h->buckets;

  q = (buckets[index]);


  /*check to see if the key / value pair already exists in the queue
    if so, update the value
   */

  size = cq_size(q);
  i = 0;
  
  while (i < size){
    p = cq_peek(q);
    comp = strcmp(p->key, key);
    if (comp == 0){
      p->value = value;
      return;
    }
    i++;
  }
						  
  /*if there is no match, we need to create a new pair*/
  new_key = malloc( (key_len+1) * sizeof(char) );
  if(!new_key) return;

  new_value = malloc( sizeof(void *) );
  if(!new_value) return;

  p = malloc(sizeof(pair));
  if(!p){
    free(new_key);
    free(new_value);
    return;
  }

  

  p->key = new_key;
  p->value = new_value;
  strcpy(p->key, key);
  p->value = value;

  /*enqueue the new key value pair*/
  cq_enq(q,p);

}

/*
Gets a value from the hashtable specified by key.

Hashes the Key, uses this as an index to find the desired value.

hash and key must be non-null. returns null if not found
 */

extern void *ht_get(hash h, char *key){

  unsigned long index;
  cirq q;
  cirq *buckets;
  pair *p;
  int i, size,comp;

  if (!h  || !key ) return NULL;

  


  index = (unsigned long) ht_hash(key)%(h->size);
  buckets = (cirq *)h->buckets;

  q = buckets[index];
  size = cq_size(q);

   i = 0;
   while (i < size){
    p = cq_peek(q);
    comp = strcmp(p->key, key);
    if (comp == 0){
      return p->value;
    }
    cq_rot(q);
    i++;
  }

   return NULL;


}


/* Provides a hash code for the given key string
The function should be strange enough to prevent duplcates in a large hash space
 */
unsigned long ht_hash(char *str)
{
	unsigned long h = 2343;
	unsigned int c;

	while ( (c = *str++) ) {
		h = ((h << 5) + h) + c;
	}
	return h;
}

#endif
