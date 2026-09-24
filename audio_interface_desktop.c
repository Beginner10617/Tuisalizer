#include "audio_interface.h"
#include "logging.h"
#include "raylib/src/raylib.h"
#include <stdbool.h>
#include <stdlib.h>
struct AudioBackend {
  Music music;
  bool initialized;
  bool loaded;
  char *path;
  float volume;
};

AudioBackend *audio_create() {
  AudioBackend *out = malloc(sizeof(AudioBackend));
  if (out == NULL)
    LogError("Unable to create audio backend - desktop");
  return out;
}

bool audio_init(AudioBackend *audio_backend) {
  InitAudioDevice();
  if (IsAudioDeviceReady())
    audio_backend->initialized = true;
  return audio_backend->initialized;
}

void audio_shutdown(AudioBackend *audio_backend) { CloseAudioDevice(); }

bool audio_load(AudioBackend *audio_backend, char *path) {
  audio_backend->music = LoadMusicStream(path);
  audio_backend->path = path;
  audio_backend->loaded = IsMusicValid(audio_backend->music);
  return audio_backend->loaded;
}

void audio_unload(AudioBackend *audio_backend) {
  UnloadMusicStream(audio_backend->music);
}

void audio_play(AudioBackend *audio_backend) {
  SetMusicVolume(audio_backend->music, audio_backend->volume);
  if (IsMusicValid(audio_backend->music))
    PlayMusicStream(audio_backend->music);
}

void audio_pause(AudioBackend *audio_backend) {
  if (IsMusicValid(audio_backend->music))
    PauseMusicStream(audio_backend->music);
}

void audio_resume(AudioBackend *audio_backend) {
  SetMusicVolume(audio_backend->music, audio_backend->volume);
  if (IsMusicValid(audio_backend->music))
    ResumeMusicStream(audio_backend->music);
}

void audio_stop(AudioBackend *audio_backend) {
  if (IsMusicValid(audio_backend->music))
    StopMusicStream(audio_backend->music);
}

void audio_update(AudioBackend *audio_backend) {
  if (IsMusicValid(audio_backend->music))
    UpdateMusicStream(audio_backend->music);
}

void audio_seek(AudioBackend *audio_backend, double seconds) {
  if (IsMusicValid(audio_backend->music))
    SeekMusicStream(audio_backend->music, (float)seconds);
}

double audio_position(AudioBackend *audio_backend) {
  if (IsMusicValid(audio_backend->music))
    return GetMusicTimePlayed(audio_backend->music);
  return 0.0;
}

double audio_duration(AudioBackend *audio_backend) {
  if (IsMusicValid(audio_backend->music))
    return GetMusicTimeLength(audio_backend->music);
  return 1.0;
}

void audio_set_volume(AudioBackend *audio_backend, float volume) {
  audio_backend->volume = volume;
  if (IsMusicValid(audio_backend->music))
    SetMusicVolume(audio_backend->music, volume);
}
