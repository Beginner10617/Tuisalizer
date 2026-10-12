#include "decoder.h"
#include "logging.h"
#include <SDL.h>
#include <stdio.h>
int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <filepath>\n", argv[0]);
    return 1;
  }
  LogInit("log_tmp.log");
  ffmpeg_decoder *decoder = decoder_create();
  if (decoder_load(decoder, argv[1])) {
    printf("Error loading the file %s\n", argv[1]);
    return 1;
  }
  float *buf = malloc(sizeof(float) * 4096);
  size_t sz;

  SDL_AudioDeviceID device;
  SDL_AudioSpec spec = {0};

  spec.freq = decoder_sample_rate(decoder); // Must match decoder sample rate
  spec.format = AUDIO_F32SYS;               // 32-bit float samples
  spec.channels = 1;                        // Mono
  spec.samples = 4096;                      // SDL buffer size
  spec.callback = NULL;                     // Use queued audio

  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
    return 1;
  }

  device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
  if (device == 0) {
    fprintf(stderr, "SDL_OpenAudioDevice: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_PauseAudioDevice(device, 0); // Start playback

  while (!decoder_finished(decoder)) {
    size_t size = 0;

    if (decoder_sample(decoder, buf, 4096, &size) != 0) {
      fprintf(stderr, "Audio decoding failed\n");
      break;
    }

    if (size > 0) {
      SDL_QueueAudio(device, buf, (Uint32)(size * sizeof(float)));
    }

    // Avoid queuing unlimited audio ahead of playback.
    while (SDL_GetQueuedAudioSize(device) > 16384) {
      SDL_Delay(10);
    }
  }

  decoder_cleanup(decoder);
  LogClose();
  return 0;
}
