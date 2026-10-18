#include "../tui/tui.h"
#include "ui.h"
#include <SDL.h>

int main() {
  UI_state ui;
  if (ui_init(&ui)) {
    printf("Initialization failed, check logs\n");
    return 1;
  }
  FrameLimiter limiter;
  frame_limiter_init(ui.fps, &limiter);
  while (ui.running) {
    ui_update(&ui);
    ui_render(&ui);
    frame_limiter_wait(&limiter);
  }
  ui_close(&ui);
  return 0;
}
