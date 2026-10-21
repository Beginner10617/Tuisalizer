#include "filesys.h"
#include <assert.h>
#include <dirent.h>
#include <string.h>

fs_dir fs_create() {
  fs_dir out;
  out.capacity = 1;
  out.count = 0;
  out.path = NULL;
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

  char *resolved = realpath(path, NULL);
  assert(resolved);

  free(dir->path);
  dir->path = resolved;

  DIR *dir_ = opendir(dir->path);
  struct dirent *x;

  while ((x = readdir(dir_)) != NULL) {
    if (strlen(x->d_name) == 1 && strncmp(".", x->d_name, 1) == 0)
      continue;
    fs_entry tmp = {.name = strdup(x->d_name), .kind = x->d_type};
    assert(tmp.name);
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
  if (!dir || !name)
    return NULL;

  size_t dir_len = strlen(dir);
  size_t name_len = strlen(name);

  char *out = malloc(dir_len + name_len + 2);
  if (!out)
    return NULL;

  memcpy(out, dir, dir_len);
  out[dir_len] = '/';
  memcpy(out + dir_len + 1, name, name_len + 1);

  return out;
}

bool starts_with(const char *str, const char *prefix) {
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

char *file_extension(const char *file_name) {
  int i = strlen(file_name) - 1;
  int j;
  for (j = i; j >= 0 && file_name[j] != '.'; j--)
    ;
  char *out = malloc(sizeof(char) * (i - j + 2));
  for (int k = 0; k < i - j + 2; k++)
    out[k] = file_name[j + k];
  return out;
}
