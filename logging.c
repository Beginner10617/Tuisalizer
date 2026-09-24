#include "logging.h"
#include <stdarg.h>
#include <stdio.h>

void LogInfo(const char *str, ...) {
  va_list args;
  va_start(args, str);
  printf("INFO: ");
  vprintf(str, args);
  printf("\n");
  va_end(args);
}
void LogDebug(const char *str, ...) {
  va_list args;
  va_start(args, str);
  printf("DEBUG: ");
  vprintf(str, args);
  printf("\n");
  va_end(args);
}
void LogError(const char *str, ...) {
  va_list args;
  va_start(args, str);
  printf("ERROR: ");
  vprintf(str, args);
  printf("\n");
  va_end(args);
}
