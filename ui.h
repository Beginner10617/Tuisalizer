#ifndef UI
#define UI
#include "tui/tui.h"
enum { FOCUS_NORMAL, FOCUS_QUEUE, FOCUS_ADD, FOCUS_SAVE, FOCUS_QUIT };
typedef struct {
  const char *track_name, *album_name, *artist_name;
  int played_seconds, duration_seconds;
  int focus, focus_button_id;

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
  uint8_t bgclr, fgclr;
} button;

void ui_init(UI_state *);
void ui_update(UI_state *);
void ui_render(UI_state *);
void ui_close(UI_state *);
void ui_render_button(button, UI_state *);
#endif
