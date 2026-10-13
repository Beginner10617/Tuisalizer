#ifndef MUSIC_H
#define MUSIC_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  float seconds_start;
  float seconds_end;
  const char *line;
} lyrics_entry;

typedef struct {
  const char *filepath;
  lyrics_entry *entries;
  size_t entry_count;
} lyrics;

typedef struct {
  const char *audio_file_path;

  // Metadata
  const char *title;
  const char *artist;
  const char *album;
  const char *album_artist;
  const char *genre;

  uint32_t duration_seconds;
  uint16_t track_number;
  uint16_t year;

  // Optional associated subtitles/lyrics
  lyrics *track_lyrics;
} track;

int load_track_metadata(const char *path, track *out);
void free_track_metadata(track *t);
#endif
