#include "logging.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/samplefmt.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  LogInit("logs.log");
  if (argc < 2) {
    LogError("Usage: %s <audio-file>", argv[0]);
    LogClose();
    return 1;
  }

  const char *filename = argv[1];

  // Open the input file

  AVFormatContext *fmt = NULL;

  if (avformat_open_input(&fmt, filename, NULL, NULL) < 0) {
    LogError("Could not open file");
    LogClose();
    return 1;
  }

  if (avformat_find_stream_info(fmt, NULL) < 0) {
    LogError("Could not find stream information");
    avformat_close_input(&fmt);
    LogClose();
    return 1;
  }

  // Find the audio stream

  int audio_stream = -1;

  for (unsigned i = 0; i < fmt->nb_streams; i++) {
    if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {

      audio_stream = i;
      break;
    }
  }

  if (audio_stream == -1) {
    LogError("No audio stream found");
    avformat_close_input(&fmt);
    LogClose();
    return 1;
  }

  AVStream *stream = fmt->streams[audio_stream];

  // Find decoder

  const AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);

  if (!decoder) {
    LogError("Decoder not found");
    avformat_close_input(&fmt);
    LogClose();
    return 1;
  }

  AVCodecContext *codec = avcodec_alloc_context3(decoder);

  if (!codec) {
    LogError("Could not allocate codec context");
    avformat_close_input(&fmt);
    LogClose();
    return 1;
  }

  if (avcodec_parameters_to_context(codec, stream->codecpar) < 0) {
    LogError("Could not copy codec parameters");
    avcodec_free_context(&codec);
    avformat_close_input(&fmt);
    LogClose();
    return 1;
  }

  if (avcodec_open2(codec, decoder, NULL) < 0) {
    LogError("Could not open decoder");
    avcodec_free_context(&codec);
    avformat_close_input(&fmt);
    LogClose();
    return 1;
  }

  LogInfo("Sample rate : %d Hz", codec->sample_rate);
  LogInfo("Channels    : %d", codec->ch_layout.nb_channels);
  LogInfo("Format      : %s", av_get_sample_fmt_name(codec->sample_fmt));

  // Allocate packet and frame

  AVPacket *packet = av_packet_alloc();
  AVFrame *frame = av_frame_alloc();

  if (!packet || !frame) {
    LogError("Could not allocate packet/frame");
    LogClose();
    return 1;
  }

  // Decode packets

  while (av_read_frame(fmt, packet) >= 0) {

    if (packet->stream_index != audio_stream) {
      av_packet_unref(packet);
      continue;
    }

    if (avcodec_send_packet(codec, packet) < 0) {
      LogError("Error sending packet");
      break;
    }

    while (1) {

      int ret = avcodec_receive_frame(codec, frame);

      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
      }

      if (ret < 0) {
        LogError("Error decoding audio");
        goto cleanup;
      }

      // We now have decoded PCM audio

      LogInfo("Decoded frame: %d samples/channel", frame->nb_samples);

      LogInfo("Format: %s", av_get_sample_fmt_name(frame->format));

      // Example for FLOAT planar audio.

      if (frame->format == AV_SAMPLE_FMT_FLTP) {

        float *left = (float *)frame->data[0];

        float *right =
            (frame->ch_layout.nb_channels > 1) ? (float *)frame->data[1] : NULL;

        for (int i = 0; i < frame->nb_samples; i++) {

          LogInfo("%f", left[i]);

          if (right)
            LogInfo(" %f", right[i]);
        }
      }
    }

    av_packet_unref(packet);
  }

cleanup:

  av_frame_free(&frame);
  av_packet_free(&packet);

  avcodec_free_context(&codec);
  avformat_close_input(&fmt);
  LogClose();

  return 0;
}
