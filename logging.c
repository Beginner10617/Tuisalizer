#include "logging.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

FILE *log_file;

void LogInit(const char *path) {
  log_file = fopen(path, "w");
  assert(log_file);
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
}
void LogError(const char *str, ...) {
  va_list args;
  va_start(args, str);
  fprintf(log_file, "ERROR: ");
  vfprintf(log_file, str, args);
  fprintf(log_file, "\n");
  va_end(args);
}
