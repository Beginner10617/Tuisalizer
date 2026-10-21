// To insert metadata in mp3 files
// Example format:
// PATH title
// /Users/myuser/tmp.mp3 siren

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage : %s <filepath>\n", argv[0]);
    return 1;
  }
  FILE *file = fopen(argv[1], "r");
  assert(file);
  char c = 1;
  size_t cols = 0;
  bool have_path = false, spaces = true;
  int path_id = 0;
  while (c != '\n') {
    assert(fscanf(file, "%c", &c) != EOF);
    if (spaces && !isspace((unsigned char)c)) {
      spaces = false;
      continue;
    }
    if (!spaces && isspace((unsigned char)c)) {
      cols++;
      spaces = true;
    }
  }
  char **arg_names = malloc(sizeof(char *) * cols);
  fseek(file, 0, SEEK_SET); // Back to beginning
  c = ' ';
  for (int i = 0; i < cols; i++) {
    while (isspace((unsigned char)c))
      assert(fscanf(file, "%c", &c) != EOF);
    size_t sz = 1;
    while (!isspace((unsigned char)c)) {
      sz++;
      assert(fscanf(file, "%c", &c) != EOF);
    }
    char *tmp = malloc(sizeof(char) * (sz));
    fseek(file, -sz, SEEK_CUR);
    for (int j = 0; j < sz; j++)
      assert(fscanf(file, "%c", &tmp[j]) != EOF);
    tmp[sz - 1] = 0;
    arg_names[i] = tmp;
    if (strncmp("PATH", tmp, strlen(tmp)) == 0) {
      have_path = true;
      path_id = i;
    }
  }
  if (!have_path) {
    for (int i = 0; i < cols; i++)
      free(arg_names[i]);
    free(arg_names);
    fclose(file);
    printf("PATH column absent in the file %s\n", argv[1]);
    return 1;
  }

  char **args = malloc(sizeof(char *) * cols);
  bool running = true;
  while (running) {
    for (int i = 0; i < cols && running; i++) {
      c = ' ';
      while (isspace((unsigned char)c))
        assert(fscanf(file, "%c", &c) != EOF);
      size_t sz = 1;
      while (!isspace(c)) {
        sz++;
        assert(fscanf(file, "%c", &c) != EOF);
      }
      char *tmp = malloc(sizeof(char) * (sz));
      fseek(file, -sz, SEEK_CUR);
      for (int j = 0; j < sz; j++)
        assert(fscanf(file, "%c", &tmp[j]) != EOF);
      tmp[sz - 1] = 0;
      if (i == 0 && strncmp(tmp, "END", strlen(tmp)) == 0)
        running = false;
      else {
        args[i] = tmp;
      }
    }
    if (running) {
      size_t sz = 55 + 16 * cols + strlen(args[path_id]) + 7;
      for (int k = 0; k < cols; k++)
        sz += strlen(arg_names[k]) + strlen(args[k]);
      char *cmd = calloc(sz + 1, sizeof(char));
      assert(cmd);

      strncpy(cmd, "ffmpeg -i ", 10);
      strncpy(cmd + 10, args[path_id], strlen(args[path_id]));
      strncpy(cmd + 10 + strlen(args[path_id]),
              " \\\n-map 0:a \\\n-c:a copy \\\n-id3v2_version 3 \\\n", 45);

      size_t locn = 55 + strlen(args[path_id]);
      for (int k = 0; k < cols; k++) {
        if (k == path_id)
          continue;
        strncpy(cmd + locn, "-metadata ", 10);
        locn += 10;
        strncpy(cmd + locn, arg_names[k], strlen(arg_names[k]));
        locn += strlen(arg_names[k]);
        strncpy(cmd + locn, "=\"", 2);
        locn += 2;
        strncpy(cmd + locn, args[k], strlen(args[k]));
        locn += strlen(args[k]);
        strncpy(cmd + locn, "\" \\\n", 4);
        locn += 4;
      }
      strncpy(cmd + locn, args[path_id], strlen(args[path_id]));
      locn += strlen(args[path_id]) - 4;
      strncpy(cmd + locn, "_tagged.mp3", 11);
      int ret = system(cmd);
      printf("%s\nexecution return value = %d\n\n", cmd, ret);
      free(cmd);
      // ffmpeg -i [PATH] \
  -map 0:a \
  -c:a copy \
  -id3v2_version 3 \
  -metadata [COLMNAME]="[COLMVALUE]" \
  -metadata publisher="Napalm Records" \
  [PATH][:-4]_tagged.mp3
    }
  }
  fclose(file);
  return 0;
}
