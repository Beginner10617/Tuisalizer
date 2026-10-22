#include "music.h"
#include <assert.h>
#include <errno.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// helper
char *morph_dash(char *in) {
  char *out = calloc(1 + strlen(in), sizeof(char));
  assert(out);
  char prev, curr = 0;
  int j = 0;
  for (int i = 0; i < strlen(in); i++) {
    prev = curr;
    curr = in[i];
    if (prev != '-' && curr == '-') {
      out[j++] = ' ';
    } else if (prev == '-' && curr == '-') {
      out[--j] = '-';
      j++;
    } else
      out[j++] = in[i];
  }
  free(in);
  return out;
}

char *metadata_dup(AVDictionary *tags, const char *key) {
  if (!tags || !key)
    return NULL;

  AVDictionaryEntry *entry = av_dict_get(tags, key, NULL, 0);

  if (!entry || !entry->value)
    return NULL;

  return strdup(entry->value);
}

static unsigned parse_uint(const char *s) {
  if (!s || !*s)
    return 0;

  errno = 0;
  char *end;
  unsigned long value = strtoul(s, &end, 10);

  if (errno || end == s || value > UINT32_MAX)
    return 0;

  return (unsigned)value;
}

/*
 * Returns 0 on success, negative on failure.
 * Call free_track_metadata() when finished.
 */
int load_track_metadata(const char *path, track *out) {
  if (!path || !out)
    return -1;

  AVFormatContext *fmt = NULL;

  int ret = avformat_open_input(&fmt, path, NULL, NULL);

  if (ret < 0)
    return ret;

  ret = avformat_find_stream_info(fmt, NULL);
  if (ret < 0) {
    avformat_close_input(&fmt);
    return ret;
  }

  AVDictionary *tags = fmt->metadata;

  out->title = morph_dash(metadata_dup(tags, "title"));
  out->artist = morph_dash(metadata_dup(tags, "artist"));
  out->album = morph_dash(metadata_dup(tags, "album"));
  out->album_artist = morph_dash(metadata_dup(tags, "album_artist"));
  out->genre = morph_dash(metadata_dup(tags, "genre"));
  // morph
  out->audio_file_path = path;

  if (fmt->duration != AV_NOPTS_VALUE && fmt->duration >= 0) {
    out->duration_seconds = (uint32_t)(fmt->duration / AV_TIME_BASE);
  }

  char *value = metadata_dup(tags, "track");
  unsigned n = parse_uint(value);
  if (n <= UINT16_MAX)
    out->track_number = (uint16_t)n;
  free(value);

  value = metadata_dup(tags, "date");
  if (!value)
    value = metadata_dup(tags, "year");

  unsigned y = parse_uint(value);
  if (y <= UINT16_MAX)
    out->year = (uint16_t)y;
  free(value);

  avformat_close_input(&fmt);
  return 0;
}

void free_track_metadata(track *m) {
  if (!m)
    return;

  free(m->title);
  free(m->artist);
  free(m->album);
  free(m->album_artist);
  free(m->genre);

  memset(m, 0, sizeof(*m));
}
