#ifndef LOGGING
#define LOGGING
#include <stdarg.h>
void LogInit(const char *path);
void LogInfo(const char *str, ...);
void LogDebug(const char *str, ...);
void LogError(const char *str, ...);
void LogFlush();
void LogClose();

static void ffmpeg_log_callback(void *avcl, int level, const char *fmt,
                                va_list vl);
#endif
