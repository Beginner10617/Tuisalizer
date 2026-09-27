#ifndef LOGGING
#define LOGGING
void LogInit(const char *path);
void LogInfo(const char *str, ...);
void LogDebug(const char *str, ...);
void LogError(const char *str, ...);
void LogClose();
#endif
