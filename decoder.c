#include "decoder.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/samplefmt.h>
typedef struct {
  AVFormatContext *fmt;
  int audio_stream;
  AVStream *stream;
  const AVCodec *decoder;
  AVCodecContext *codec;
  AVPacket *packet;
  AVFrame *frame;
} ffmpeg_decoder;
