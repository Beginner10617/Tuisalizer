#include "queue.h"
#include <stdio.h>
int main() {
  queue que;
  queue_init(&que, sizeof(int));
  int x1 = 5, x2 = 6, x3 = 7;
  queue_enque(&que, &x1);
  queue_enque(&que, &x2);
  queue_enque(&que, &x3);
  int y1, y2, y3;
  queue_deque(&que, &y1);
  queue_deque(&que, &y2);
  queue_deque(&que, &y3);
  printf("y : [%d %d %d]\nx : [%d %d %d]\n", y1, y2, y3, x1, x2, x3);
}
