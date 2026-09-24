#ifndef AUDIO_INTERFACE
#define AUDIO_INTERFACE
#include <stdbool.h>
typedef struct AudioBackend AudioBackend;

AudioBackend *audio_create();
bool audio_init(AudioBackend *);
void audio_shutdown(AudioBackend *);

bool audio_load(AudioBackend *, char *path);
void audio_unload(AudioBackend *);
void audio_play(AudioBackend *);
void audio_pause(AudioBackend *);
void audio_resume(AudioBackend *);
void audio_stop(AudioBackend *);

void audio_update(AudioBackend *);

void audio_seek(AudioBackend *, double seconds);
double audio_position(AudioBackend *);
double audio_duration(AudioBackend *);

void audio_set_volume(AudioBackend *, float volume);
#endif
