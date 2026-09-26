#define TUI_IMPLEMENTATION
#include "ui.h"
#include "tui/tui.h"
#define SCREEN_WIDTH 127
#define SCREEN_HEIGHT 34
#define FPS 30

void ui_init(UI_state *ui) {
  ui->mode = MODE_PAUSED;
  ui->window = createTermWindow(SCREEN_WIDTH, SCREEN_HEIGHT);
  ui->fps = FPS;
}

void ui_update(UI_state *ui) {}

void ui_render(UI_state *ui) {
  fill_clr(BLACK, &ui->window);

  // basic layout
  Rect rect = {0, 0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1};
  set_color_fg(WHITE, &ui->window);
  draw_borders(rect, &ui->window);
  move_cursor(0, 0, &ui->window);
  write_char(u'╭', &ui->window);
  move_cursor(0, SCREEN_WIDTH - 1, &ui->window);
  write_char(u'╮', &ui->window);
  move_cursor(SCREEN_HEIGHT - 1, 0, &ui->window);
  write_char(u'╰', &ui->window);
  move_cursor(SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, &ui->window);
  write_char(u'╯', &ui->window);

  move_cursor(1, 1, &ui->window);
  write_str("  TUISALIZER", &ui->window);
  move_cursor(2, 0, &ui->window);
  write_char(u'├', &ui->window);
  move_cursor(2, SCREEN_WIDTH - 1, &ui->window);
  write_char(u'┤', &ui->window);
  for (int i = 1; i < SCREEN_WIDTH - 1; i++) {
    move_cursor(2, i, &ui->window);
    write_char(u'─', &ui->window);
  }

  // render call
  display(&ui->window);
}
