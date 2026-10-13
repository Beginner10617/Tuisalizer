#include "queue.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void queue_init(queue *que, size_t elem_size) {
  que->elem_size = elem_size;
  que->size = 0;
  que->cap = 1;
  que->head = 0;
  que->values = malloc(elem_size);
  assert(que->values);
}
void queue_enque(queue *que, void *elem) {
  if (que->size == que->cap) {
    size_t new_cap = que->cap * 2;
    void *tmp = malloc(que->elem_size * new_cap);
    assert(tmp);
    // move values int tmp
    for (size_t i = 0; i < que->size; i++) {
      size_t old_index = (que->head + i) % que->cap;

      memcpy((char *)tmp + i * que->elem_size,
             (char *)que->values + old_index * que->elem_size, que->elem_size);
    }
    free(que->values);
    que->values = tmp;
    que->cap = new_cap;
    que->head = 0;
  }
  size_t index = (que->head + que->size) % que->cap;

  memcpy((char *)que->values + que->elem_size * index, elem, que->elem_size);
  que->size++;
}

void queue_deque(queue *que, void *dst) {
  if (que->size == 0)
    return;
  memcpy(dst, (char *)que->values + que->elem_size * que->head, que->elem_size);
  que->size--;
  que->head = (que->head + 1) % que->cap;
}
void queue_destroy(queue *que) { free(que->values); }
