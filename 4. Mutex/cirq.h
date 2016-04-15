/* implementation of a circular queue, By Dan Kenefick
*/

#ifndef CIRQ_H
#define CIRQ_H
typedef struct element **cirq;

typedef struct element
{
  struct element* next;
  void *data;
} element;

extern cirq cq_alloc(void);
extern int cq_size(cirq q);
extern void cq_enq(cirq q, void *value);
extern void *cq_deq(cirq q);
extern void *cq_peek(cirq q);
extern void cq_rot(cirq q);
extern void cq_free(cirq q);

#endif
