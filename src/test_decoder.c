#include "decoder.h"
#include <stdio.h>
int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <filepath>\n", argv[0]);
    return 1;
  }
  ffmpeg_decoder *decoder = decoder_create();
  if (decoder_load(decoder, argv[1])) {
    printf("Error loading the file %s\n", argv[1]);
    return 1;
  }
  float *buf = malloc(sizeof(float) * 4096);
  size_t sz;
  decoder_sample(decoder, buf, 4096, &sz);
  for (size_t i = 0; i < sz; i++)
    printf("%f\n", buf[i]);
  decoder_cleanup(decoder);
  return 0;
}
