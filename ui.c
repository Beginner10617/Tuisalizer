#define TUI_IMPLEMENTATION
#include "ui.h"
#include "tui/tui.h"
#define SCREEN_WIDTH 115
#define SCREEN_HEIGHT 30
#define FPS 30
#define BUTTON_COUNT 7
#define LOWER_PANEL_BUTTON_COUNT 7
#define LOWER_PANEL_BUTTON_WIDTH (SCREEN_WIDTH - 2) / LOWER_PANEL_BUTTON_COUNT
#define BUTTON_BGCLR_DEFAULT BLACK
#define BUTTON_FGCLR_DEFAULT WHITE
#define BUTTON_BGCLR_HIGHLIGHT BLACK
#define BUTTON_FGCLR_HIGHLIGHT WHITE

button Buttons[BUTTON_COUNT] = {
    {{"[SPACE] Play", "[SPACE] Pause"},
     SCREEN_HEIGHT - 2,
     1,
     0,
     false,
     BUTTON_BGCLR_DEFAULT,
     BUTTON_BGCLR_HIGHLIGHT,
     BUTTON_FGCLR_DEFAULT,
     BUTTON_FGCLR_HIGHLIGHT},

    {{"[←→] Seek", ""},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_WIDTH * 1,
     0,
     false,
     BUTTON_BGCLR_DEFAULT,
     BUTTON_BGCLR_HIGHLIGHT,
     BUTTON_FGCLR_DEFAULT,
     BUTTON_FGCLR_HIGHLIGHT},

    {{"[↑↓] Volume", ""},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_WIDTH * 2,
     0,
     false,
     BUTTON_BGCLR_DEFAULT,
     BUTTON_BGCLR_HIGHLIGHT,
     BUTTON_FGCLR_DEFAULT,
     BUTTON_FGCLR_HIGHLIGHT},

    {{"[L] Loop", "[L] Close Loop"},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_WIDTH * 3,
     0,
     false,
     BUTTON_BGCLR_DEFAULT,
     BUTTON_BGCLR_HIGHLIGHT,
     BUTTON_FGCLR_DEFAULT,
     BUTTON_FGCLR_HIGHLIGHT},

    {{"[A] Add", "[-] Add"},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_WIDTH * 4,
     0,
     false,
     BUTTON_BGCLR_DEFAULT,
     BUTTON_BGCLR_HIGHLIGHT,
     BUTTON_FGCLR_DEFAULT,
     BUTTON_FGCLR_HIGHLIGHT},

    {{"[S] Save", ""},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_WIDTH * 5,
     0,
     false,
     BUTTON_BGCLR_DEFAULT,
     BUTTON_BGCLR_HIGHLIGHT,
     BUTTON_FGCLR_DEFAULT,
     BUTTON_FGCLR_HIGHLIGHT},

    {{"[Q] Quit", ""},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_WIDTH * 6,
     0,
     false,
     BUTTON_BGCLR_DEFAULT,
     BUTTON_BGCLR_HIGHLIGHT,
     BUTTON_FGCLR_DEFAULT,
     BUTTON_FGCLR_HIGHLIGHT},

};

void ui_init(UI_state *ui) {
  ui->mode = MODE_PAUSED;
  ui->window = createTermWindow(SCREEN_WIDTH, SCREEN_HEIGHT);
  ui->fps = FPS;
  ui->running = true;
  enable_raw_mode();
}

void ui_update(UI_state *ui) {
  tui_poll_events(&ui->inputs);
  if (ui->inputs.pressed[TUIK_CHAR]) {
    switch (ui->inputs.c_data) {
    case 'q':
    case 'Q':
      ui->running = false;
      break;
    default:
      break;
    }
  }
}

void ui_render(UI_state *ui) {
  fill_clr(BLACK, &ui->window);

  // basic layout
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

  move_cursor(SCREEN_HEIGHT - 5, 3, &ui->window);
  write_str("QUEUE", &ui->window);

  move_cursor(SCREEN_HEIGHT - 6, 0, &ui->window);
  write_char(u'├', &ui->window);
  move_cursor(SCREEN_HEIGHT - 6, SCREEN_WIDTH - 1, &ui->window);
  write_char(u'┤', &ui->window);
  for (int i = 1; i < SCREEN_WIDTH - 1; i++) {
    move_cursor(SCREEN_HEIGHT - 6, i, &ui->window);
    write_char(u'─', &ui->window);
  }

  move_cursor(SCREEN_HEIGHT - 3, 0, &ui->window);
  write_char(u'├', &ui->window);
  move_cursor(SCREEN_HEIGHT - 3, SCREEN_WIDTH - 1, &ui->window);
  write_char(u'┤', &ui->window);
  for (int i = 1; i < SCREEN_WIDTH - 1; i++) {
    move_cursor(SCREEN_HEIGHT - 3, i, &ui->window);
    write_char(u'─', &ui->window);
  }

  for (int i = 0; i < BUTTON_COUNT; i++)
    ui_render_button(Buttons[i], ui);

  // render call
  display(&ui->window);
}

void ui_close(UI_state *ui) { show_cursor(); }

void ui_render_button(button b, UI_state *ui) {
  move_cursor(b.row, b.col, &ui->window);
  if (b.highlighted) {
    set_color_bg(b.bgclr_highlight, &ui->window);
    set_color_fg(b.fgclr_higlight, &ui->window);
  } else {
    set_color_bg(b.bgclr_default, &ui->window);
    set_color_fg(b.fgclr_default, &ui->window);
  }
  write_str(b.texts[b.mode], &ui->window);
}
