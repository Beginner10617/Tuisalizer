#include "logging.h"
#include <assert.h>
#include <libavutil/log.h>
#include <stdio.h>
#include <string.h>
FILE *log_file;

void LogInit(const char *path) {
  log_file = fopen(path, "w");
  assert(log_file);
  av_log_set_callback(ffmpeg_log_callback);
  av_log_set_level(AV_LOG_INFO);
}

void LogClose() {
  if (log_file)
    fclose(log_file);
}

void LogInfo(const char *str, ...) {
  va_list args;
  va_start(args, str);
  fprintf(log_file, "INFO: ");
  vfprintf(log_file, str, args);
  fprintf(log_file, "\n");
  va_end(args);
}
void LogDebug(const char *str, ...) {
  va_list args;
  va_start(args, str);
  fprintf(log_file, "DEBUG: ");
  vfprintf(log_file, str, args);
  fprintf(log_file, "\n");
  va_end(args);
  LogFlush();
}
void LogError(const char *str, ...) {
  va_list args;
  va_start(args, str);
  fprintf(log_file, "ERROR: ");
  vfprintf(log_file, str, args);
  fprintf(log_file, "\n");
  va_end(args);
}
void LogFlush() { fflush(log_file); }

static void ffmpeg_log_callback(void *avcl, int level, const char *fmt,
                                va_list vl) {
  if (level > av_log_get_level())
    return;

  char message[1024];
  vsnprintf(message, sizeof(message), fmt, vl);

  // FFmpeg messages often end with a newline.
  message[strcspn(message, "\r\n")] = '\0';

  if (message[0] == '\0')
    return;

  if (level <= AV_LOG_ERROR) {
    LogError("[FFmpeg] %s", message);
  } else if (level <= AV_LOG_WARNING) {
    LogInfo("[FFmpeg] %s", message);
  } else {
    LogDebug("[FFmpeg] %s", message);
  }
}
