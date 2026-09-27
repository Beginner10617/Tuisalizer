#include "filesys.h"
#include <assert.h>
#include <dirent.h>
#include <string.h>

fs_dir fs_create() {
  fs_dir out;
  out.capacity = 1;
  out.count = 0;
  out.entries = malloc(sizeof(fs_entry));
  assert(out.entries);
  return out;
}

void fs_append(fs_dir *dir, fs_entry entry) {
  if (dir->count == dir->capacity) {
    dir->capacity *= 2;
    dir->entries = realloc(dir->entries, sizeof(fs_entry) * dir->capacity);
    assert(dir->entries);
  }
  dir->entries[dir->count] = entry;
  dir->count++;
}

int fs_read_dir(const char *path, fs_dir *dir) {
  // overwrite previous
  dir->count = 0;

  DIR *dir_ = opendir(".");
  struct dirent *x;
  while (x) {
    x = readdir(dir_);
    if (!x)
      break;
    fs_entry tmp = {x->d_name, x->d_type};
    fs_append(dir, tmp);
  }
  return dir->count;
}

void fs_free_dir(fs_dir *dir) {
  dir->capacity = 0;
  dir->count = 0;
  free(dir->entries);
}

char *fs_join_path(const char *dir, const char *name) {
  int out_sz = strlen(dir) + strlen(name) + 1;
  char *out = malloc(sizeof(char) * out_sz);
  for (int i = 0; i < strlen(dir); i++)
    out[i] = dir[i];
  out[strlen(dir)] = '/';
  for (int i = 0; i < strlen(name); i++)
    out[i + strlen(dir) + 1] = name[i];
  return out;
}
