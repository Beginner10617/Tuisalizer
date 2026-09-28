#ifndef FILE_SYS
#define FILE_SYS
#include <dirent.h>
#include <stdlib.h>
enum {
  KIND_DIR = DT_DIR,
  KIND_FILE = DT_REG,
};
typedef struct fs_entry {
  char *name;
  int kind;
} fs_entry;

typedef struct fs_dir {
  fs_entry *entries;
  char *path;
  size_t count, capacity;
} fs_dir;

fs_dir fs_create();
void fs_append(fs_dir *, fs_entry);
int fs_read_dir(const char *path, fs_dir *dir);
void fs_free_dir(fs_dir *dir);
char *fs_join_path(const char *dir, const char *name);
#endif
