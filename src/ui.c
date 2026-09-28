#include "filesys.h"
#include <stdint.h>
#include <string.h>
#define TUI_IMPLEMENTATION
#include "../tui/tui.h"
#include "logging.h"
#include "ui.h"
#define SCREEN_WIDTH 115
#define SCREEN_HEIGHT 30
#define FPS 30
#define BUTTON_COUNT 8
#define LOWER_PANEL_BUTTON_COUNT 7
#define LOWER_PANEL_BUTTON_WIDTH (SCREEN_WIDTH - 2) / LOWER_PANEL_BUTTON_COUNT

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
                                 LIME,
                                 BLACK},

                                {{"[←→] Seek", ""},
                                 SCREEN_HEIGHT - 2,
                                 1 + LOWER_PANEL_BUTTON_WIDTH * 1,
                                 0,
                                 false,
                                 SILVER,
                                 BLACK},

                                {{"[↑↓] Volume", ""},
                                 SCREEN_HEIGHT - 2,
                                 1 + LOWER_PANEL_BUTTON_WIDTH * 2,
                                 0,
                                 false,
                                 TEAL,
                                 BLACK},

                                {{"[L] Loop", "[L] UnLoop"},
                                 SCREEN_HEIGHT - 2,
                                 1 + LOWER_PANEL_BUTTON_WIDTH * 3,
                                 0,
                                 false,
                                 PURPLE,
                                 BLACK},

                                {{"[A] Add", "[-] Add"},
                                 SCREEN_HEIGHT - 2,
                                 1 + LOWER_PANEL_BUTTON_WIDTH * 4,
                                 0,
                                 false,
                                 AQUA,
                                 BLACK},

                                {{"[S] Save", "[-] Save"},
                                 SCREEN_HEIGHT - 2,
                                 1 + LOWER_PANEL_BUTTON_WIDTH * 5,
                                 0,
                                 false,
                                 BLUE,
                                 BLACK},

                                {{"[Q] Quit", "[-] Quit"},
                                 SCREEN_HEIGHT - 2,
                                 1 + LOWER_PANEL_BUTTON_WIDTH * 6,
                                 0,
                                 false,
                                 RED,
                                 BLACK},

                                {{"[I] Queue", "[-] Queue"},
                                 SCREEN_HEIGHT - 5,
                                 3,
                                 0,
                                 false,
                                 YELLOW,
                                 BLACK}};

void ui_init(UI_state *ui) {
  LogInit("logs.log");
  ui->focus = FOCUS_NORMAL;
  ui->window = createTermWindow(SCREEN_WIDTH, SCREEN_HEIGHT);
  ui->fps = FPS;
  ui->running = true;
  ui->search_buf[0] = 0;
  ui->search_buf_index = 0;
  ui->file_sys = fs_create();
  ui->cursor_posn = 0;
  fs_read_dir(".", &ui->file_sys);
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
  if (ui->focus == FOCUS_QUIT && ui->inputs.pressed[TUIK_ENTER])
    ui->running = false;
  if (ui->focus == FOCUS_ADD) {
    if (ui->inputs.pressed[TUIK_CHAR] &&
        ui->search_buf_index < sizeof(ui->search_buf) - 1) {
      ui->cursor_posn = 0;
      ui->search_buf[ui->search_buf_index++] = ui->inputs.c_data;
      ui->search_buf[ui->search_buf_index] = 0;
    } else if (ui->inputs.pressed[TUIK_SPACE] &&
               ui->search_buf_index < sizeof(ui->search_buf) - 1) {
      ui->cursor_posn = 0;
      ui->search_buf[ui->search_buf_index++] = ' ';
    } else if (ui->inputs.pressed[TUIK_BACK] && ui->search_buf_index >= 0) {
      if (ui->search_buf_index > 0)
        ui->cursor_posn = 0;
      ui->search_buf_index--;
      ui->search_buf[ui->search_buf_index] = 0;
    }
    if (ui->inputs.pressed[TUIK_UP] && ui->cursor_posn > 0)
      ui->cursor_posn--;
    else if (ui->inputs.pressed[TUIK_DOWN] &&
             ui->cursor_posn + 1 < ui->file_sys.count)
      ui->cursor_posn++;
    if (ui->inputs.pressed[TUIK_ENTER])
      LogInfo("current selected file : %s", ui->curr_selected_entry.name);
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

  // focus mode display
  char *mode_str = "NORMAL";
  uint8_t bg_clr = GREEN;
  switch (ui->focus) {
  case FOCUS_QUIT:
    mode_str = "QUIT";
    bg_clr = RED;
    break;
  case FOCUS_QUEUE:
    mode_str = "QUEUE";
    bg_clr = YELLOW;
    break;
  case FOCUS_ADD:
    mode_str = "ADD";
    bg_clr = AQUA;
    break;
  case FOCUS_SAVE:
    mode_str = "SAVE";
    bg_clr = BLUE;
    break;
  }
  move_cursor(1, SCREEN_WIDTH - strlen(mode_str) - 3, &ui->window);
  set_color_bg(bg_clr, &ui->window);
  set_color_fg(BLACK, &ui->window);
  write_str(mode_str, &ui->window);
  set_color_bg(BLACK, &ui->window);
  set_color_fg(WHITE, &ui->window);

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

  // focus window rendering
  if (ui->focus == FOCUS_QUIT) {
    int s_row = SCREEN_HEIGHT / 2 - 2, s_col = SCREEN_WIDTH / 2 - 18;
    Rect tmp = {s_row, s_col, s_row + 4, s_col + 35};
    draw_rounded_borders(tmp, &ui->window);
    move_cursor(s_row + 1, s_col + 3, &ui->window);
    write_str("Are you sure you want to quit?", &ui->window);
    move_cursor(s_row + 3, s_col + 3, &ui->window);
    write_str("YES [ENTER]        NO [ESCAPE]", &ui->window);
  } else if (ui->focus == FOCUS_ADD) {
    int s_row = SCREEN_HEIGHT / 2 - 10, s_col = SCREEN_WIDTH / 2 - 18;
    Rect tmp = {s_row, s_col, s_row + 18, s_col + 35};
    draw_rounded_borders(tmp, &ui->window);

    move_cursor(s_row + 1, s_col + 2, &ui->window);
    write_str("ADD", &ui->window);

    move_cursor(s_row + 3, s_col, &ui->window);
    write_char(u'├', &ui->window);
    move_cursor(s_row + 3, s_col + 35, &ui->window);
    write_char(u'┤', &ui->window);

    int posn = 0, start, end;
    start = (ui->cursor_posn >= 12 ? ui->cursor_posn - 11 : 0);
    end = (ui->cursor_posn >= 12 ? ui->cursor_posn + 1 : 12);
    for (int i = 0; i < ui->file_sys.count && posn < end; i++) {
      if (!starts_with(ui->file_sys.entries[i].name, ui->search_buf) &&
          strncmp("..", ui->file_sys.entries[i].name, 2))
        continue;
      if (posn < start) {
        posn++;
        continue;
      }
      char *tmp_c = file_extension(ui->file_sys.entries[i].name);
      if (ui->cursor_posn == posn) {
        ui->curr_selected_entry = ui->file_sys.entries[i];
        set_color_bg(GREY, &ui->window);
      } else {
        set_color_bg(BLACK, &ui->window);
      }
      if ((strncmp(tmp_c, ".mp3", 3) == 0) ||
          (strncmp(tmp_c, ".srt", 3) == 0) ||
          (strncmp(tmp_c, ".plist", 3) == 0) ||
          (ui->file_sys.entries[i].kind == KIND_DIR)) {
        set_color_fg(WHITE, &ui->window);
      } else {
        if (ui->cursor_posn == posn)
          set_color_fg(BLACK, &ui->window);
        else
          set_color_fg(GREY, &ui->window);
      }
      int row = s_row + 4 + posn - start;
      move_cursor(row, s_col + 1, &ui->window);
      if (posn == ui->cursor_posn)
        write_str(ui->file_sys.entries[i].name, &ui->window);
      else
        write_str_prefix(ui->file_sys.entries[i].name, 32, &ui->window);

      free(tmp_c);
      posn++;
    }
    set_color_bg(BLACK, &ui->window);
    set_color_fg(WHITE, &ui->window);

    move_cursor(s_row + 17, s_col + 2, &ui->window);
    write_str_suffix(ui->search_buf, 31, &ui->window);
    move_cursor(s_row + 17,
                s_col + 2 +
                    (ui->search_buf_index < 31 ? ui->search_buf_index : 30),
                &ui->window);
    set_color_bg(GREY, &ui->window);
    write_char(' ', &ui->window);
    set_color_bg(BLACK, &ui->window);

    move_cursor(s_row + 16, s_col, &ui->window);
    write_char(u'├', &ui->window);
    move_cursor(s_row + 16, s_col + 35, &ui->window);
    write_char(u'┤', &ui->window);
    for (int i = s_col + 1; i < s_col + 35; i++) {
      move_cursor(s_row + 3, i, &ui->window);
      write_char(u'─', &ui->window);
      move_cursor(s_row + 16, i, &ui->window);
      write_char(u'─', &ui->window);
    }

  } else if (ui->focus == FOCUS_SAVE) {

  } else if (ui->focus == FOCUS_QUEUE) {
    // no separate rect per-se, but a cursor to navigate
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
