#include "decoder.h"
#include <assert.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
struct ffmpeg_decoder {
  AVFormatContext *fmt;
  int audio_stream;
  AVStream *stream;
  const AVCodec *decoder;
  AVCodecContext *codec;
  AVPacket *packet;
  AVFrame *frame;
  SwrContext *swr;

  float *buffer;
  size_t buffer_size;
  size_t buffer_offset;

  int eof;
  int finished;
  int error;

  double position;
};

ffmpeg_decoder *decoder_create(void) {
  ffmpeg_decoder *output = calloc(1, sizeof(*output));
  assert(output);
  output->audio_stream = -1;
  return output;
}

void decoder_cleanup(ffmpeg_decoder *decoder) {
  av_frame_free(&decoder->frame);
  av_packet_free(&decoder->packet);
  avcodec_free_context(&decoder->codec);
  avformat_close_input(&decoder->fmt);
}

int decoder_load(ffmpeg_decoder *decoder, const char *filepath) {
  if (avformat_open_input(&decoder->fmt, filepath, NULL, NULL) < 0)
    return 1;

  if (avformat_find_stream_info(decoder->fmt, NULL) < 0) {
    avformat_close_input(&decoder->fmt);
    return 1;
  }

  // Find the audio stream

  decoder->audio_stream = -1;

  for (unsigned i = 0; i < decoder->fmt->nb_streams; i++) {
    if (decoder->fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      decoder->audio_stream = i;
      break;
    }
  }

  if (decoder->audio_stream == -1) {
    avformat_close_input(&decoder->fmt);
    return 1;
  }

  decoder->stream = decoder->fmt->streams[decoder->audio_stream];

  // Find decoder

  decoder->decoder = avcodec_find_decoder(decoder->stream->codecpar->codec_id);

  if (!decoder->decoder) {
    avformat_close_input(&decoder->fmt);
    return 1;
  }

  decoder->codec = avcodec_alloc_context3(decoder->decoder);

  if (!decoder->codec) {
    avformat_close_input(&decoder->fmt);
    return 1;
  }

  if (avcodec_parameters_to_context(decoder->codec, decoder->stream->codecpar) <
      0) {
    avcodec_free_context(&decoder->codec);
    avformat_close_input(&decoder->fmt);
    return 1;
  }

  if (avcodec_open2(decoder->codec, decoder->decoder, NULL) < 0) {
    avcodec_free_context(&decoder->codec);
    avformat_close_input(&decoder->fmt);
    return 1;
  }

  // Allocate packet and frame

  decoder->packet = av_packet_alloc();
  decoder->frame = av_frame_alloc();

  if (!decoder->packet || !decoder->frame)
    return 1;
  AVChannelLayout out_layout = AV_CHANNEL_LAYOUT_MONO;

  int ret = swr_alloc_set_opts2(
      &decoder->swr, &out_layout, AV_SAMPLE_FMT_FLT,
      decoder->codec->sample_rate, &decoder->codec->ch_layout,
      decoder->codec->sample_fmt, decoder->codec->sample_rate, 0, NULL);

  if (ret < 0 || !decoder->swr || swr_init(decoder->swr) < 0) {
    decoder_unload(decoder);
    return 1;
  }
  return 0;
}

void decoder_unload(ffmpeg_decoder *decoder) {
  if (!decoder)
    return;

  free(decoder->buffer);
  decoder->buffer = NULL;
  decoder->buffer_size = 0;
  decoder->buffer_offset = 0;

  swr_free(&decoder->swr);
  av_frame_free(&decoder->frame);
  av_packet_free(&decoder->packet);
  avcodec_free_context(&decoder->codec);
  avformat_close_input(&decoder->fmt);

  decoder->stream = NULL;
  decoder->decoder = NULL;
  decoder->audio_stream = -1;

  decoder->eof = 0;
  decoder->finished = 0;
  decoder->error = 0;
}

int decoder_sample(ffmpeg_decoder *decoder, float *data_out, size_t capacity,
                   size_t *size) {
  if (!decoder || !data_out || !size)
    return 1;

  *size = 0;

  while (*size < capacity) {
    // Consume buffered samples first.
    if (decoder->buffer_offset < decoder->buffer_size) {
      size_t available = decoder->buffer_size - decoder->buffer_offset;
      size_t count = capacity - *size;

      if (count > available)
        count = available;

      memcpy(data_out + *size, decoder->buffer + decoder->buffer_offset,
             count * sizeof(float));

      decoder->buffer_offset += count;
      *size += count;
      continue;
    }

    free(decoder->buffer);
    decoder->buffer = NULL;
    decoder->buffer_size = 0;
    decoder->buffer_offset = 0;

    if (decoder->error)
      return 1;

    if (decoder->finished)
      break;

    // Receive decoded frames before reading more packets.
    int ret = avcodec_receive_frame(decoder->codec, decoder->frame);

    if (ret == 0) {
      // track timestamp
      AVStream *stream = decoder->fmt->streams[decoder->audio_stream];

      if (decoder->frame->best_effort_timestamp != AV_NOPTS_VALUE) {
        int64_t start = stream->start_time;

        if (start == AV_NOPTS_VALUE)
          start = 0;

        decoder->position = (decoder->frame->best_effort_timestamp - start) *
                            av_q2d(stream->time_base);
      }

      int max_samples = av_rescale_rnd(
          swr_get_delay(decoder->swr, decoder->codec->sample_rate) +
              decoder->frame->nb_samples,
          decoder->codec->sample_rate, decoder->codec->sample_rate,
          AV_ROUND_UP);

      decoder->buffer = malloc((size_t)max_samples * sizeof(float));
      if (!decoder->buffer) {
        av_frame_unref(decoder->frame);
        decoder->error = 1;
        return 1;
      }

      uint8_t *out[] = {(uint8_t *)decoder->buffer};

      int converted =
          swr_convert(decoder->swr, out, max_samples,
                      (const uint8_t **)decoder->frame->extended_data,
                      decoder->frame->nb_samples);

      av_frame_unref(decoder->frame);

      if (converted < 0) {
        free(decoder->buffer);
        decoder->buffer = NULL;
        decoder->error = 1;
        return 1;
      }

      // Mono output: one float per sample.
      decoder->buffer_size = (size_t)converted;
      decoder->buffer_offset = 0;
      continue;
    }

    if (ret == AVERROR_EOF) {
      decoder->finished = 1;
      break;
    }

    if (ret != AVERROR(EAGAIN)) {
      decoder->error = 1;
      return 1;
    }

    // The decoder needs another packet.
    if (decoder->eof) {
      decoder->error = 1; // Unexpected EAGAIN after flushing.
      return 1;
    }

    while ((ret = av_read_frame(decoder->fmt, decoder->packet)) >= 0) {
      if (decoder->packet->stream_index != decoder->audio_stream) {
        av_packet_unref(decoder->packet);
        continue;
      }

      ret = avcodec_send_packet(decoder->codec, decoder->packet);
      av_packet_unref(decoder->packet);

      if (ret == AVERROR(EAGAIN))
        break; // Receive pending frames before retrying this packet.

      if (ret < 0) {
        decoder->error = 1;
        return 1;
      }

      break;
    }

    if (ret == AVERROR(EAGAIN))
      continue;

    if (ret == AVERROR_EOF)
      continue;

    if (ret < 0) {
      // End of input: flush delayed decoder output.
      decoder->eof = 1;

      ret = avcodec_send_packet(decoder->codec, NULL);
      if (ret < 0 && ret != AVERROR_EOF) {
        decoder->error = 1;
        return 1;
      }
    }
  }

  return 0;
}

int decoder_seek(ffmpeg_decoder *decoder, double seconds) {
  if (!decoder || !decoder->fmt || !decoder->codec ||
      decoder->audio_stream < 0 || seconds < 0.0) {
    return -1;
  }

  AVStream *stream = decoder->fmt->streams[decoder->audio_stream];

  // Convert seconds to the audio stream's timestamp units.
  int64_t timestamp = (int64_t)(seconds / av_q2d(stream->time_base));

  // Seek backward to a suitable decoding point.
  int ret = av_seek_frame(decoder->fmt, decoder->audio_stream, timestamp,
                          AVSEEK_FLAG_BACKWARD);

  if (ret < 0) {
    return ret;
  }

  // Discard buffered decoder state from the previous position.
  avcodec_flush_buffers(decoder->codec);

  // Discard any samples already converted and buffered.
  free(decoder->buffer);
  decoder->buffer = NULL;
  decoder->buffer_size = 0;
  decoder->buffer_offset = 0;

  // Reset resampler state.
  if (decoder->swr) {
    swr_close(decoder->swr);

    ret = swr_init(decoder->swr);
    if (ret < 0) {
      decoder->error = ret;
      return ret;
    }
  }

  // Clear end-of-stream and error state.
  decoder->eof = 0;
  decoder->finished = 0;
  decoder->error = 0;

  return 0;
}

double decoder_position(const ffmpeg_decoder *decoder) {
  if (!decoder || !decoder->fmt || decoder->audio_stream < 0)
    return -1.0;

  return decoder->position;
}

double decoder_duration(const ffmpeg_decoder *decoder) {
  if (!decoder || !decoder->fmt) {
    return -1.0;
  }

  // Format duration is expressed in AV_TIME_BASE units.
  if (decoder->fmt->duration != AV_NOPTS_VALUE && decoder->fmt->duration >= 0) {
    return (double)decoder->fmt->duration / AV_TIME_BASE;
  }

  // Fall back to the audio stream's duration.
  if (decoder->audio_stream >= 0) {
    AVStream *stream = decoder->fmt->streams[decoder->audio_stream];

    if (stream->duration != AV_NOPTS_VALUE && stream->duration >= 0) {
      return stream->duration * av_q2d(stream->time_base);
    }
  }

  return -1.0;
}

int decoder_sample_rate(ffmpeg_decoder *decoder) {
  return decoder->codec->sample_rate;
}
int decoder_finished(ffmpeg_decoder *decoder) { return decoder->finished; }
