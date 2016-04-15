
/* code for a circular queue, by Dan Kenefick*/

#ifndef CIRQ_C
#define CIRQ_C

#include "cirq.h"
#include <stdlib.h>
#include <stdio.h>



/* allocate a new cirular queue
returns the newly allocated queue, pointing to a NULL pointer to an element. 
 */
extern cirq cq_alloc(void){

  cirq q;
  q = (element**) malloc( sizeof(element*));

  return q;

}

/*
Returns the number of elements within the queue 
counts by transversing until a null "next" pointer in some element is found.
 */

extern int cq_size(cirq q){

  element *e = *q;
  int i;
  if(!e) return 0;
  i = 1;

  while ((*e).next){
    e = (*e).next;
    i++;
  }

  return i;

}

/*
Enqueue an element to the end of the queue
transverses the queue, adding an element at the end.
 */

extern void cq_enq(cirq q, void *value){

  element* new;
  new = (element *) malloc ( sizeof(element) );
  if (!new) return;

  new->data = malloc ( sizeof( void *) );
  if (!new->data){
    free(new->data);
    free(new);
    return;
  }

  new->data = value;
	new->next = NULL;
	
  if(!(*q)){
    *q = (new);
  } else{
    

    /*need to fix this so it actualy updates next*/
    element* e = (*q);
    while(e->next){
      e = e->next;
    }
    e->next = new;
  }
  return;

}

/*
Dequeue an element.
removes the head of the queue, changed the cirq pointer to the head of the next
element, releases the element from memory, returns a copy of element's pointer. 
 */

extern void *cq_deq(cirq q){

  element *ep;
  void *pointer;

  if( !(*q) ){
    return NULL;
  }

  ep = *q;

  pointer = ep->data;
	
	if(ep->next){
		*q = ep->next;
	} else{
		*q = NULL;
	}
  
  /*Frees the mem from the old head of the queue*/
  free (ep);
	memset(ep,0,sizeof(element));
	

  return pointer; 

}

/*
Returns the next element in the queue without removing it. the pointer is not a copy.
 */

extern void *cq_peek(cirq q){
  element *ep;
  void *pointer;

  if( !(*q) ){
    return NULL;
  }
  ep = *q; 
  pointer = ep->data;

  return pointer; 

}

/*
Rotates the queue
enqueus the head of the queue at the tail.
 */

extern void cq_rot(cirq q){
  element *first;
  element *second;
  if( !(*q) ){
    return;
  }

  first = *q;
  second = first->next;

  /*if the list is of lenth 1, do nothing*/
  if (!second) return;
  *q = second;

  while(second->next){
    second = second->next;
  }

  second->next = first;
  
  first->next = NULL;
	
	return;
	
}

/*
Removes the entire queue
dequeues all of the elements, which also frees them. Then frees the ascociated data.  
 */

extern void cq_free(cirq q){


  while ( (cq_deq(q)) ){
  }

  /* so q isn't pointing to anything nasty later*/
  free(q);
  q = NULL;

  return;

}

#endif
