#ifndef QUEUE_H
#define QUEUE_H
#include <stdlib.h>
typedef struct {
  void *values;
  size_t elem_size, size, cap, head;
} queue;
void queue_init(queue *que, size_t elem_size);
void queue_enque(queue *que, void *elem);
void queue_deque(queue *que, void *dst);
void queue_destroy(queue *que);
#endif
