#define TUI_IMPLEMENTATION
#include "ui.h"
#include "logging.h"
#include "tui/tui.h"
#define SCREEN_WIDTH 115
#define SCREEN_HEIGHT 30
#define FPS 30
#define BUTTON_COUNT 8
#define LOWER_PANEL_BUTTON_COUNT 7
#define LOWER_PANEL_BUTTON_WIDTH (SCREEN_WIDTH - 2) / LOWER_PANEL_BUTTON_COUNT
#define BUTTON_BGCLR BLACK
#define BUTTON_FGCLR WHITE

#define BUTTON_INDEX_PAUSE_PLAY 0
#define BUTTON_INDEX_SEEK 1
#define BUTTON_INDEX_VOLUME 2
#define BUTTON_INDEX_LOOP 3
#define BUTTON_INDEX_ADD 4
#define BUTTON_INDEX_SAVE 5
#define BUTTON_INDEX_QUIT 6
#define BUTTON_INDEX_QUEUE 7

button Buttons[BUTTON_COUNT] = {{{"[SPACE] Play", "[SPACE] Pause"},
                                 SCREEN_HEIGHT - 2,
                                 1,
                                 0,
                                 false,
                                 BUTTON_BGCLR,
                                 BUTTON_FGCLR},

                                {
                                    {"[←→] Seek", ""},
                                    SCREEN_HEIGHT - 2,
                                    1 + LOWER_PANEL_BUTTON_WIDTH * 1,
                                    0,
                                    false,
                                    BUTTON_BGCLR,
                                    BUTTON_FGCLR,
                                },

                                {
                                    {"[↑↓] Volume", ""},
                                    SCREEN_HEIGHT - 2,
                                    1 + LOWER_PANEL_BUTTON_WIDTH * 2,
                                    0,
                                    false,
                                    BUTTON_BGCLR,
                                    BUTTON_FGCLR,
                                },

                                {
                                    {"[L] Loop", "[L] Close Loop"},
                                    SCREEN_HEIGHT - 2,
                                    1 + LOWER_PANEL_BUTTON_WIDTH * 3,
                                    0,
                                    false,
                                    BUTTON_BGCLR,
                                    BUTTON_FGCLR,
                                },

                                {
                                    {"[A] Add", "[-] Add"},
                                    SCREEN_HEIGHT - 2,
                                    1 + LOWER_PANEL_BUTTON_WIDTH * 4,
                                    0,
                                    false,
                                    BUTTON_BGCLR,
                                    BUTTON_FGCLR,
                                },

                                {
                                    {"[S] Save", "[-] Save"},
                                    SCREEN_HEIGHT - 2,
                                    1 + LOWER_PANEL_BUTTON_WIDTH * 5,
                                    0,
                                    false,
                                    BUTTON_BGCLR,
                                    BUTTON_FGCLR,
                                },

                                {{"[Q] Quit", "[-] Quit"},
                                 SCREEN_HEIGHT - 2,
                                 1 + LOWER_PANEL_BUTTON_WIDTH * 6,
                                 0,
                                 false,
                                 BUTTON_BGCLR,
                                 BUTTON_FGCLR},

                                {
                                    {"[I] Queue", "[-] Queue"},
                                    SCREEN_HEIGHT - 5,
                                    3,
                                    0,
                                    false,
                                    BUTTON_BGCLR,
                                    BUTTON_FGCLR,
                                }};

void ui_init(UI_state *ui) {
  LogInit("logs.log");
  ui->focus = FOCUS_NORMAL;
  ui->window = createTermWindow(SCREEN_WIDTH, SCREEN_HEIGHT);
  ui->fps = FPS;
  ui->running = true;
  enable_raw_mode();
}

void ui_update(UI_state *ui) {
  tui_poll_events(&ui->inputs);
  if (ui->inputs.pressed[TUIK_CHAR] && ui->focus == FOCUS_NORMAL) {
    switch (ui->inputs.c_data) {
    case 'i':
    case 'I':
      ui->focus = FOCUS_QUEUE;
      ui->focus_button_id = BUTTON_INDEX_QUEUE;
      Buttons[BUTTON_INDEX_QUEUE].mode = 1;
      LogInfo("switching to focus mode queue");
      break;
    case 'a':
    case 'A':
      ui->focus = FOCUS_ADD;
      ui->focus_button_id = BUTTON_INDEX_ADD;
      Buttons[BUTTON_INDEX_ADD].mode = 1;
      LogInfo("switching to focus mode add");
      break;
    case 's':
    case 'S':
      ui->focus = FOCUS_SAVE;
      ui->focus_button_id = BUTTON_INDEX_SAVE;
      Buttons[BUTTON_INDEX_SAVE].mode = 1;
      LogInfo("switching to focus mode save");
      break;
    case 'q':
    case 'Q':
      ui->focus = FOCUS_QUIT;
      ui->focus_button_id = BUTTON_INDEX_QUIT;
      Buttons[BUTTON_INDEX_QUIT].mode = 1;
      LogInfo("switching to focus mode quit");
      ui->running = false; // to be removed afterwards
      break;
    default:
      break;
    }
  }
  if (ui->inputs.pressed[TUIK_ESCAPE] && ui->focus != FOCUS_NORMAL) {
    LogInfo("switching to focus mode normal");
    ui->focus = FOCUS_NORMAL;
    Buttons[ui->focus_button_id].mode = 0;
    ui->focus_button_id = -1;
  }
}

void ui_render(UI_state *ui) {
  fill_clr(BLACK, &ui->window);

  // Outer boundary
  Rect rect = {0, 0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1};
  set_color_fg(WHITE, &ui->window);
  draw_rounded_borders(rect, &ui->window);
  move_cursor(1, 3, &ui->window);
  write_str("TUISALIZER", &ui->window);

  move_cursor(2, 0, &ui->window);
  write_char(u'├', &ui->window);
  move_cursor(2, SCREEN_WIDTH - 1, &ui->window);
  write_char(u'┤', &ui->window);
  for (int i = 1; i < SCREEN_WIDTH - 1; i++) {
    move_cursor(2, i, &ui->window);
    write_char(u'─', &ui->window);
  }

  // Queue window
  move_cursor(SCREEN_HEIGHT - 5, 3, &ui->window);
  write_str("[I] QUEUE", &ui->window);

  move_cursor(SCREEN_HEIGHT - 6, 0, &ui->window);
  write_char(u'├', &ui->window);
  move_cursor(SCREEN_HEIGHT - 6, SCREEN_WIDTH - 1, &ui->window);
  write_char(u'┤', &ui->window);
  for (int i = 1; i < SCREEN_WIDTH - 1; i++) {
    move_cursor(SCREEN_HEIGHT - 6, i, &ui->window);
    write_char(u'─', &ui->window);
  }

  // lower panel for buttons
  move_cursor(SCREEN_HEIGHT - 3, 0, &ui->window);
  write_char(u'├', &ui->window);
  move_cursor(SCREEN_HEIGHT - 3, SCREEN_WIDTH - 1, &ui->window);
  write_char(u'┤', &ui->window);
  for (int i = 1; i < SCREEN_WIDTH - 1; i++) {
    move_cursor(SCREEN_HEIGHT - 3, i, &ui->window);
    write_char(u'─', &ui->window);
  }

  // rendering buttons
  for (int i = 0; i < BUTTON_COUNT; i++)
    ui_render_button(Buttons[i], ui);

  // display call
  display(&ui->window);
}

void ui_close(UI_state *ui) {
  show_cursor();
  LogClose();
}

void ui_render_button(button b, UI_state *ui) {
  move_cursor(b.row, b.col, &ui->window);
  set_color_bg(b.bgclr, &ui->window);
  set_color_fg(b.fgclr, &ui->window);
  write_str(b.texts[b.mode], &ui->window);
}
