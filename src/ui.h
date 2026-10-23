#ifndef UI
#define UI
#include "../tui/tui.h"
#include "decoder.h"
#include "filesys.h"
#include "music.h"
#include "queue.h"
#include <SDL.h>
enum { FOCUS_NORMAL, FOCUS_QUEUE, FOCUS_ADD, FOCUS_QUIT };
typedef enum {
  PLAYER_STOPPED,
  PLAYER_PLAYING,
  PLAYER_PAUSED,
  PLAYER_FINISHED,
} playback_mode;

typedef struct {
  const char *track_name, *album_name, *artist_name;
  int played_seconds, duration_seconds;
  int focus, focus_button_id;

  // tui state
  TerminalWindow window;
  unsigned int fps;
  InputState inputs;
  bool running;

  // backend
  fs_dir file_sys;

  // search buffers
  char search_buf[256];
  size_t search_buf_index;
  size_t cursor_posn;
  fs_entry curr_selected_entry;

  // music display data
  queue music_queue;
  playback_mode p_mode;
  track current_track;
  int loop_mode;

  // queue window
  size_t current_queue_selection;

  // music decoding
  ffmpeg_decoder *decoder;
  int decoder_change_flags; // MSB, ..., position changed ?, track changed ?
  float *pcm_buf, volume;

  // sdl music backend
  SDL_AudioDeviceID device;
  SDL_AudioSpec spec;
} UI_state;

typedef struct {
  const char *texts[3];
  int row, col, mode;
  bool highlighted;
  uint8_t bgclr, fgclr;
} button;

int ui_init(UI_state *);
void ui_update(UI_state *);
void ui_render(UI_state *);
void ui_close(UI_state *);
void ui_render_button(button, UI_state *);
void enque_from_plist(queue *m_que, const char *filepath);
#endif
