#ifndef DECODER
#define DECODER
#include <stdlib.h>
typedef struct ffmpeg_decoder ffmpeg_decoder;

// constructor - destructor
ffmpeg_decoder *decoder_create(void);
void decoder_cleanup(ffmpeg_decoder *);

int decoder_load(ffmpeg_decoder *, const char *filepath);
void decoder_unload(ffmpeg_decoder *);

int decoder_sample(ffmpeg_decoder *, float *data_out, size_t capacity,
                   size_t *size);

int decoder_seek(ffmpeg_decoder *, double seconds);
double decoder_position(const ffmpeg_decoder *);
double decoder_duration(const ffmpeg_decoder *);

int decoder_finished(const ffmpeg_decoder *);
int decoder_error(const ffmpeg_decoder *);
#endif
