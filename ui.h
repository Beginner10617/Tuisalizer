#ifndef UI
#define UI
#include "tui/tui.h"
enum {
  MODE_PAUSED = 1 << 0,
  MODE_LOOP = 1 << 1,
  MODE_VISUALIZE = 1 << 2,
};
typedef struct {
  const char *track_name, *album_name, *artist_name;
  int played_seconds, duration_seconds;
  int mode;

  // tui state
  TerminalWindow window;
  unsigned int fps;
  InputState inputs;
  bool running;
} UI_state;

typedef struct {
  const char *texts[2];
  int row, col, mode;
  bool highlighted;
  uint8_t bgclr_default, bgclr_highlight, fgclr_default, fgclr_higlight;
} button;

void ui_init(UI_state *);
void ui_update(UI_state *);
void ui_render(UI_state *);
void ui_close(UI_state *);
void ui_render_button(button, UI_state *);
#endif
