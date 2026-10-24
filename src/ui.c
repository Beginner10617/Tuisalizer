#include "decoder.h"
#define FFT_IMPLEMENTATION
#include "fft.h"
#include "filesys.h"
#include "music.h"
#include "queue.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define TUI_IMPLEMENTATION
#define SDL_QUEUE_GAP 16384
#include "../tui/tui.h"
#include "logging.h"
#include "ui.h"
#define SCREEN_WIDTH 115
#define SCREEN_HEIGHT 30
#define FPS 30
#define BUTTON_COUNT 7
#define LOWER_PANEL_BUTTON_COUNT 6
#define LOWER_PANEL_BUTTON_WIDTH (SCREEN_WIDTH - 2) / LOWER_PANEL_BUTTON_COUNT
#define LOWER_PANEL_BUTTON_OFFSET 7
#define BUTTON_INDEX_PAUSE_PLAY 0
#define BUTTON_INDEX_SEEK 1
#define BUTTON_INDEX_VOLUME 2
#define BUTTON_INDEX_LOOP 3
#define BUTTON_INDEX_ADD 4
#define BUTTON_INDEX_QUIT 5
#define BUTTON_INDEX_QUEUE 6
#define TIMELINE_SEEK_STEP 15 // in sec
#define VOLUME_DIVSIONS 20
#define QUEUE_TRACK_DISPLAY_WIDTH 15

// helper
char *to_time_str(int sec);

button Buttons[BUTTON_COUNT] = {
    {{"[SPACE] Play", "[SPACE] Pause"},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_OFFSET,
     0,
     false,
     LIME,
     BLACK},

    {{"[←→] Seek", ""},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_WIDTH * 1 + LOWER_PANEL_BUTTON_OFFSET,
     0,
     false,
     SILVER,
     BLACK},

    {{"[↑↓] Volume", ""},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_WIDTH * 2 + LOWER_PANEL_BUTTON_OFFSET,
     0,
     false,
     TEAL,
     BLACK},

    {{"[L] Loop one", "[L] Un-Loop", "[L] Loop all"},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_WIDTH * 3 + LOWER_PANEL_BUTTON_OFFSET,
     0,
     false,
     PURPLE,
     BLACK},

    {{"[A] Add", "[-] Add"},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_WIDTH * 4 + LOWER_PANEL_BUTTON_OFFSET,
     0,
     false,
     AQUA,
     BLACK},

    {{"[Q] Quit", "[-] Quit"},
     SCREEN_HEIGHT - 2,
     1 + LOWER_PANEL_BUTTON_WIDTH * 5 + LOWER_PANEL_BUTTON_OFFSET,
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

int ui_init(UI_state *ui) {
  LogInit("logs.log");
  ui->focus = FOCUS_NORMAL;
  ui->window = createTermWindow(SCREEN_WIDTH, SCREEN_HEIGHT);
  ui->fps = FPS;
  ui->running = true;
  ui->visualizer_enabled = true;

  ui->search_buf[0] = 0;
  ui->search_buf_index = 0;
  ui->file_sys = fs_create();
  ui->cursor_posn = 0;
  fs_read_dir(".", &ui->file_sys);
  queue_init(&ui->music_queue, sizeof(track));

  ui->p_mode = PLAYER_STOPPED;
  ui->loop_mode = false;
  ui->decoder = decoder_create();
  ui->volume = 1.0;
  ui->current_queue_selection = 0;

  ui->pcm_buf = malloc(sizeof(float) * 4096);
  ui->bar_height = calloc(110, sizeof(int));
  ui->bar_clr = malloc(110 * sizeof(uint8_t));
  ui->bins_count = 4096;
  ui->bar_count = 110;
  ui->max_height = 34;
  assert(ui->pcm_buf && ui->bar_height && ui->bar_clr);

  // colors
  const uint8_t colors[][3] = {
      {255, 0, 0},   // Red
      {255, 255, 0}, // Yellow
      {0, 255, 0},   // Green
      {0, 255, 255}, // Cyan
      {0, 0, 255},   // Blue
  };
  const int segments = 4;
  for (int i = 0; i < ui->bar_count; i++) {
    float t = (float)i * segments / (ui->bar_count - 1);
    int segment = (int)t;
    float f = t - segment;

    if (segment >= segments) {
      segment = segments - 1;
      f = 1.0f;
    }

    uint8_t r = (uint8_t)(colors[segment][0] * (1.0f - f) +
                          colors[segment + 1][0] * f + 0.5f);

    uint8_t g = (uint8_t)(colors[segment][1] * (1.0f - f) +
                          colors[segment + 1][1] * f + 0.5f);

    uint8_t b = (uint8_t)(colors[segment][2] * (1.0f - f) +
                          colors[segment + 1][2] * f + 0.5f);

    ui->bar_clr[i] = rgb_to_ansi(r, g, b);
  }

  ui->spec.freq = 48000;
  ui->spec.format = AUDIO_F32SYS;
  ui->spec.channels = 1;
  ui->spec.samples = 4096;
  ui->spec.callback = NULL; // Queued audio

  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    LogError("SDL_Init: %s\n", SDL_GetError());
    return 1;
  }
  ui->device = SDL_OpenAudioDevice(NULL, 0, &ui->spec, NULL, 0);
  if (ui->device == 0) {
    LogError("SDL_OpenAudioDevice: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  enable_raw_mode();
  return 0;
}

void ui_update(UI_state *ui) {
  // input handling
  tui_poll_events(&ui->inputs);
  if (ui->inputs.pressed[TUIK_CHAR] && ui->focus == FOCUS_NORMAL) {
    switch (ui->inputs.c_data) {
    case 'l':
    case 'L':
      ui->loop_mode = (1 + ui->loop_mode) % 3;
      Buttons[BUTTON_INDEX_LOOP].mode = ui->loop_mode;
      break;
    case 'V':
    case 'v':
      ui->visualizer_enabled = !ui->visualizer_enabled;
      break;
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
  if (ui->focus == FOCUS_NORMAL && ui->inputs.pressed[TUIK_SPACE]) {
    if (ui->p_mode == PLAYER_PAUSED) {
      Buttons[BUTTON_INDEX_PAUSE_PLAY].mode = 0;
      ui->p_mode = PLAYER_PLAYING;
    } else if (ui->p_mode == PLAYER_PLAYING) {
      Buttons[BUTTON_INDEX_PAUSE_PLAY].mode = 1;
      ui->p_mode = PLAYER_PAUSED;
    }
  }
  Buttons[BUTTON_INDEX_SEEK].mode = (ui->p_mode == PLAYER_PLAYING) ? 0 : 1;
  if (ui->focus == FOCUS_NORMAL && ui->p_mode == PLAYER_PLAYING &&
      (ui->inputs.pressed[TUIK_LEFT] ^ ui->inputs.pressed[TUIK_RIGHT])) {
    double src = decoder_position(ui->decoder);
    double dur = decoder_duration(ui->decoder);
    if (ui->inputs.pressed[TUIK_LEFT]) {
      double dst = src > TIMELINE_SEEK_STEP ? src - TIMELINE_SEEK_STEP : 0.0;
      decoder_seek(ui->decoder, dst);
    } else if (ui->inputs.pressed[TUIK_RIGHT]) {
      double dst =
          src + TIMELINE_SEEK_STEP < dur ? src + TIMELINE_SEEK_STEP : dur;
      decoder_seek(ui->decoder, dst);
    }
  }
  if (ui->focus == FOCUS_QUEUE &&
      (ui->inputs.pressed[TUIK_LEFT] ^ ui->inputs.pressed[TUIK_RIGHT])) {
    if (ui->inputs.pressed[TUIK_LEFT] && ui->current_queue_selection)
      ui->current_queue_selection--;
    else if (ui->inputs.pressed[TUIK_RIGHT] &&
             ui->current_queue_selection + 1 < ui->music_queue.size)
      ui->current_queue_selection++;
  }
  if (ui->focus == FOCUS_QUEUE && ui->inputs.pressed[TUIK_ENTER] &&
      ui->music_queue.size) {
    track tmp;
    for (int i = 0; i < ui->current_queue_selection; i++) {
      queue_deque(&ui->music_queue, &tmp);
      if (ui->loop_mode == 2)
        queue_enque(&ui->music_queue, &tmp);
    }
    ui->p_mode = PLAYER_STOPPED;
    ui->current_queue_selection = 0;
    ui->focus = FOCUS_NORMAL;
    Buttons[BUTTON_INDEX_QUEUE].mode = 0;
  }
  if (ui->focus == FOCUS_NORMAL &&
      (ui->inputs.pressed[TUIK_UP] ^ ui->inputs.pressed[TUIK_DOWN])) {
    float VOLUME_STEP = 1.0 / VOLUME_DIVSIONS;
    if (ui->inputs.pressed[TUIK_UP]) {
      ui->volume =
          (ui->volume + VOLUME_STEP <= 1.0) ? ui->volume + VOLUME_STEP : 1.0;
    } else if (ui->inputs.pressed[TUIK_DOWN]) {
      ui->volume = (ui->volume > VOLUME_STEP) ? ui->volume - VOLUME_STEP : 0.0;
    }
  }

  if (ui->inputs.pressed[TUIK_ESCAPE] && ui->focus != FOCUS_NORMAL) {
    LogInfo("switching to focus mode normal");
    ui->focus = FOCUS_NORMAL;
    ui->current_queue_selection = 0;
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
    } else if (ui->inputs.pressed[TUIK_BACK] && ui->search_buf_index > 0) {
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
    if (ui->inputs.pressed[TUIK_ENTER]) {
      const char *name = ui->curr_selected_entry.name;
      LogInfo("current selected file : %s", name);
      if (ui->curr_selected_entry.kind == KIND_DIR) {
        char *new_path = fs_join_path(ui->file_sys.path, name);
        fs_read_dir(new_path, &ui->file_sys);
        free(new_path);
      } else if (strncmp(".mp3", file_extension(name), 4) == 0) {
        track tmp;
        char *path = fs_join_path(ui->file_sys.path, name);

        load_track_metadata(path, &tmp);
        queue_enque(&ui->music_queue, &tmp);
        LogInfo(
            "Added track %s to the queue : TITLE : %s ARTIST : %s ALBUM : %s",
            tmp.audio_file_path, tmp.title, tmp.artist, tmp.album);
        LogFlush();
        ui->focus = FOCUS_NORMAL;
        Buttons[BUTTON_INDEX_ADD].mode = 0;
      } else if (strncmp(".plist", file_extension(name), 6) == 0) {
        char *path = fs_join_path(ui->file_sys.path, name);
        enque_from_plist(&ui->music_queue, path);
        ui->focus = FOCUS_NORMAL;
        Buttons[BUTTON_INDEX_ADD].mode = 0;
      }
    }
  }

  // playback state
  if (ui->p_mode == PLAYER_FINISHED) {
    if (ui->loop_mode == 0) {
      ui->p_mode = PLAYER_PLAYING;
      ui->decoder_change_flags = 1;
    } else {
      ui->p_mode = PLAYER_STOPPED;
    }
    Buttons[BUTTON_INDEX_PAUSE_PLAY].mode = 0;
  }
  if (ui->p_mode == PLAYER_STOPPED && ui->music_queue.size) {
    queue_deque(&ui->music_queue, &ui->current_track);
    if (ui->loop_mode == 2)
      queue_enque(&ui->music_queue, &ui->current_track);
    ui->p_mode = PLAYER_PLAYING;
    Buttons[BUTTON_INDEX_PAUSE_PLAY].mode = 0;
    ui->decoder_change_flags = 1;
  }

  // music playing
  if (ui->decoder_change_flags & 1) {
    ui->decoder_change_flags &= ~1;

    decoder_unload(ui->decoder);
    SDL_ClearQueuedAudio(ui->device);

    if (decoder_load(ui->decoder, ui->current_track.audio_file_path) != 0) {
      LogError("Error loading file: %s", ui->current_track.audio_file_path);
      ui->p_mode = PLAYER_STOPPED;
    } else {
      // Configure FFmpeg resampling here so that decoder output
      // is 48000 Hz, mono, float32.
      SDL_PauseAudioDevice(ui->device, 0);
    }
    LogInfo("Decoder sample rate: %d", decoder_sample_rate(ui->decoder));

    LogInfo("Audio device: %u", (unsigned int)ui->device);
  } else if (ui->decoder_change_flags & (1 << 1)) {
    // seek to new position
  } else if (decoder_finished(ui->decoder)) {
    ui->p_mode = PLAYER_FINISHED;
  }
  // decode and queue the buffer
  if (ui->p_mode == PLAYER_PLAYING) {
    size_t size = 0;
    if (SDL_GetQueuedAudioSize(ui->device) < SDL_QUEUE_GAP) {
      if (decoder_sample(ui->decoder, ui->pcm_buf, 4096, &size) != 0) {
        LogError("Audio decoding failed, size = %zu", size);
        ui->p_mode = PLAYER_STOPPED;
        SDL_ClearQueuedAudio(ui->device);
      }
      if (size > 0) {
        set_volume(ui->pcm_buf, size, ui->volume);
        if (SDL_QueueAudio(ui->device, ui->pcm_buf,
                           (Uint32)(size * sizeof(float)))) {
          LogError("SDL_QueueAudio: %s\n", SDL_GetError());
        }
        // FFT
        if (size == ui->bins_count && ui->visualizer_enabled) {
          float complex *output = fft(ui->pcm_buf, ui->bins_count);
          int usable_size = ui->bins_count / 2;
          float max_mag = 100;
          for (int i = 0; i < ui->bar_count; i++) {
            float peak = 0;
            int start = i * usable_size / ui->bar_count;
            int end = (i + 1) * usable_size / ui->bar_count;
            if (end > ui->bins_count)
              end = ui->bins_count;
            for (int j = start; j < end; j++) {
              float mag = cabsf(output[j]);

              if (mag > peak)
                peak = mag;
            }

            ui->bar_height[i] = ui->max_height * sqrtf(peak / max_mag);
            if (ui->bar_height[i] > ui->max_height)
              ui->bar_height[i] = ui->max_height;
          }
          free(output);
        }
      } else
        LogError("Audio decoding failed");
    } else {
    }
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
  }
  move_cursor(1, SCREEN_WIDTH - utf8_strlen(mode_str) - 3, &ui->window);
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

  // Main window
  if (ui->p_mode == PLAYER_PLAYING || ui->p_mode == PLAYER_PAUSED) {

    // metadata
    if (ui->current_track.genre) {
      move_cursor(3, 3, &ui->window);
      write_str("Genre: ", &ui->window);
      write_str(ui->current_track.genre, &ui->window);
    }

    move_cursor(SCREEN_HEIGHT - 7, 3, &ui->window);
    if (ui->current_track.title) {
      write_str(ui->current_track.title, &ui->window);
    } else {
      const char *name = strrchr(ui->current_track.audio_file_path, '/');
      name = name ? name + 1 : ui->current_track.audio_file_path;
      write_str(name, &ui->window);
    }

    if (ui->current_track.artist) {
      int colm = SCREEN_WIDTH / 2 - utf8_strlen(ui->current_track.artist) / 2;
      move_cursor(SCREEN_HEIGHT - 7, colm, &ui->window);
      write_str(ui->current_track.artist, &ui->window);
    }

    // timeline
    int dur_sec = (int)decoder_duration(ui->decoder);
    int pos_sec = (int)decoder_position(ui->decoder);

    char *tmp = to_time_str(pos_sec);
    move_cursor(SCREEN_HEIGHT - 9, 3, &ui->window);
    write_str(tmp, &ui->window);
    free(tmp);

    tmp = to_time_str(dur_sec);
    move_cursor(SCREEN_HEIGHT - 9, SCREEN_WIDTH - 8, &ui->window);
    write_str(tmp, &ui->window);
    free(tmp);

    move_cursor(SCREEN_HEIGHT - 9, 9, &ui->window);
    int tot_sz = SCREEN_WIDTH - 18;
    int highlight_sz = tot_sz * pos_sec / dur_sec;
    for (int i = 0; i < tot_sz; i++) {
      move_cursor(SCREEN_HEIGHT - 9, 9 + i, &ui->window);
      if (i == highlight_sz) {
        write_char(u'●', &ui->window);
        set_color_fg(GREY, &ui->window);
      } else {
        write_char(u'━', &ui->window);
      }
    }
    set_color_fg(WHITE, &ui->window);

    // volume
    tot_sz = VOLUME_DIVSIONS;
    highlight_sz = VOLUME_DIVSIONS * ui->volume;
    for (int i = 0; i <= tot_sz; i++) {
      move_cursor(SCREEN_HEIGHT - 7, SCREEN_WIDTH - 24 + i, &ui->window);
      if (i == highlight_sz) {
        write_char(u'●', &ui->window);
        set_color_fg(GREY, &ui->window);
      } else {
        write_char(u'━', &ui->window);
      }
    }
    set_color_fg(WHITE, &ui->window);

    // Visualization

    for (int i = 1; i < ui->bar_count && ui->visualizer_enabled; i++) {
      int colm = 2 + i;
      int height = ui->bar_height[i];
      set_color_fg(ui->bar_clr[i], &ui->window);
      set_color_bg(ui->bar_clr[i], &ui->window);
      for (int j = 0; j < height / 2; j++) {
        move_cursor(SCREEN_HEIGHT - 10 - j, colm, &ui->window);
        write_char(' ', &ui->window);
      }
      if (height % 2) {
        set_color_bg(BLACK, &ui->window);
        move_cursor(SCREEN_HEIGHT - 10 - height / 2, colm, &ui->window);
        write_char(u'▄', &ui->window);
      }
    }

    set_color_fg(WHITE, &ui->window);
    set_color_bg(BLACK, &ui->window);
  } else if (ui->focus == FOCUS_NORMAL) {
    const char *message = "Click [A] to select files";
    move_cursor(SCREEN_HEIGHT / 2 - 1, SCREEN_WIDTH / 2 - strlen(message) / 2,
                &ui->window);
    write_str(message, &ui->window);
  }
  //

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
  size_t start = (ui->focus == FOCUS_QUEUE) ? ui->current_queue_selection : 0;
  int colm = 3;
  for (int i = start; i < ui->music_queue.size && colm < SCREEN_WIDTH - 1;
       i++) {
    track tck;
    memcpy(&tck,
           ui->music_queue.values +
               (i + ui->music_queue.head) * ui->music_queue.elem_size,
           ui->music_queue.elem_size);
    const char *display_str;
    if (tck.title) {
      display_str = tck.title;
    } else {
      display_str = strrchr(ui->current_track.audio_file_path, '/');
      display_str = display_str ? display_str + 1 : tck.audio_file_path;
    }
    int len = utf8_strlen(display_str);
    int wrt_sz =
        (len > QUEUE_TRACK_DISPLAY_WIDTH && i != ui->current_queue_selection)
            ? QUEUE_TRACK_DISPLAY_WIDTH
            : len;
    if (wrt_sz + colm >= SCREEN_WIDTH)
      wrt_sz = SCREEN_WIDTH - colm - 1;
    move_cursor(SCREEN_HEIGHT - 4, colm, &ui->window);
    if (ui->focus == FOCUS_QUEUE && i == ui->current_queue_selection)
      set_color_bg(TEAL, &ui->window);
    else
      set_color_bg(NAVY, &ui->window);
    write_str_prefix(display_str, wrt_sz, &ui->window);
    set_color_bg(BLACK, &ui->window);
    colm += wrt_sz + 1;
  }

  //

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

    move_cursor(s_row + 2, s_col + 2, &ui->window);
    write_str_suffix(ui->file_sys.path, 31, &ui->window);

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
      if ((strncmp(tmp_c, ".mp3", 4) == 0) ||
          (strncmp(tmp_c, ".plist", 6) == 0) ||
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
        write_str_prefix(ui->file_sys.entries[i].name, 31, &ui->window);

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
  free(ui->pcm_buf);
  queue_destroy(&ui->music_queue);
  fs_free_dir(&ui->file_sys);
  SDL_Quit();
  show_cursor();
  LogClose();
}

void ui_render_button(button b, UI_state *ui) {
  move_cursor(b.row, b.col, &ui->window);
  set_color_bg(b.bgclr, &ui->window);
  set_color_fg(b.fgclr, &ui->window);
  write_str(b.texts[b.mode], &ui->window);
}

void enque_from_plist(queue *m_que, const char *filepath) {
  FILE *file = fopen(filepath, "r");
  if (!file) {
    LogError("Unavle to open file %s", filepath);
    return;
  }
  while (1) {
    char c;
    while (fscanf(file, "%c", &c) == 1) {
      if (c == '[')
        break;
    }
    if (c != '[')
      break;
    size_t sz = 0;
    while (fscanf(file, "%c", &c) == 1) {
      if (c == ']')
        break;
      sz++;
    }
    if (c != ']')
      break;
    char *music_path = calloc(sz + 1, sizeof(char));
    assert(music_path);
    fseek(file, -sz - 1, SEEK_CUR);
    for (int i = 0; i < sz; i++)
      fscanf(file, "%c", &music_path[i]);
    fscanf(file, "%c", &c);
    assert(c == ']');
    track tmp;
    load_track_metadata(music_path, &tmp);
    queue_enque(m_que, &tmp);
  }
  fclose(file);
}

char *to_time_str(int sec) {
  char *out = calloc(6, sizeof(char));
  int m = sec / 60, s = sec % 60;
  out[2] = ':';
  out[0] = m / 10 + '0';
  out[1] = m % 10 + '0';
  out[3] = s / 10 + '0';
  out[4] = s % 10 + '0';
  return out;
}
