#include "video_info.h"
#include <libavformat/avformat.h>
#include <pthread.h>

typedef struct {
  char* in_path;
  char* out_path;
  double start;
  double end;
  GSourceFunc progress_cb;
} cut_arguments_t;

static void* cut_video_thread(void* arguments);
static gboolean cut_video_internal(const char* in_path,
                                   const char* out_path,
                                   double start,
                                   double end,
                                   GSourceFunc progress_cb);

gboolean video_duration(const char* path, double* duration) {
  AVFormatContext* format = avformat_alloc_context();
  if (avformat_open_input(&format, path, NULL, NULL) != 0) {
    return FALSE;
  }
  if (avformat_find_stream_info(format, NULL) < 0) {
    avformat_free_context(format);
    return FALSE;
  }
  *duration = ((double)format->duration) / (double)AV_TIME_BASE;
  avformat_free_context(format);
  return TRUE;
}

void cut_video(char* in_path,
               char* out_path,
               double start,
               double end,
               GSourceFunc progress_cb) {
  cut_arguments_t* args = (cut_arguments_t*)malloc(sizeof(cut_arguments_t));
  args->in_path = in_path;
  args->out_path = out_path;
  args->start = start;
  args->end = end;
  args->progress_cb = progress_cb;

  pthread_t thread;
  pthread_create(&thread, NULL, cut_video_thread, args);
  pthread_detach(thread);
}

static void* cut_video_thread(void* arguments) {
  cut_arguments_t* args = (cut_arguments_t*)arguments;
  float* value = (float*)g_malloc(sizeof(float));
  *value = PROGRESS_FAILURE;
  if (cut_video_internal(args->in_path, args->out_path, args->start, args->end,
                         args->progress_cb)) {
    *value = PROGRESS_SUCCESS;
  }
  g_main_context_invoke_full(NULL, 0, args->progress_cb, value, g_free);
  g_free(args->in_path);
  g_free(args->out_path);
  free(args);
  return NULL;
}

gboolean cut_video_internal(const char* in_path,
                            const char* out_path,
                            double start,
                            double end,
                            GSourceFunc progress_cb) {
  AVFormatContext* in_ctx = avformat_alloc_context();
  if (avformat_open_input(&in_ctx, in_path, NULL, NULL) != 0) {
    return FALSE;
  }
  if (avformat_find_stream_info(in_ctx, NULL) < 0) {
    avformat_free_context(in_ctx);
    return FALSE;
  }

  AVFormatContext* out_ctx;
  if (avformat_alloc_output_context2(&out_ctx, NULL, NULL, out_path)) {
    avformat_free_context(in_ctx);
    return FALSE;
  }

  for (int i = 0; i < in_ctx->nb_streams; ++i) {
    AVStream* in_stream = in_ctx->streams[i];
    AVStream* out_stream = avformat_new_stream(
        out_ctx, avcodec_find_encoder(in_stream->codecpar->codec_id));
    avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
    out_stream->codecpar->codec_tag = 0;
  }

  if (avio_open(&out_ctx->pb, out_path, AVIO_FLAG_WRITE) < 0) {
    goto fail;
  }
  if (avformat_init_output(out_ctx, NULL) < 0) {
    goto fail;
  }
  if (avformat_write_header(out_ctx, NULL) < 0) {
    goto fail;
  }

  int64_t start_time = (int64_t)(start * (double)AV_TIME_BASE);
  if (av_seek_frame(in_ctx, -1, start_time, 0) < 0) {
    goto fail;
  }
  AVPacket packet;
  av_init_packet(&packet);
  while (av_read_frame(in_ctx, &packet) >= 0) {
    AVRational time_base = in_ctx->streams[packet.stream_index]->time_base;
    if (packet.pts == AV_NOPTS_VALUE) {
      packet.pts = packet.dts;
    }
    double dts =
        ((double)packet.dts * (double)time_base.num) / (double)time_base.den;
    double pts =
        ((double)packet.pts * (double)time_base.num) / (double)time_base.den;

    float* progress = (float*)g_malloc(sizeof(float));
    *progress = (float)MAX(0, MIN((dts - start) / (end - start), 1));
    g_main_context_invoke_full(NULL, 0, progress_cb, progress, g_free);

    if (dts > end) {
      break;
    }

    time_base = out_ctx->streams[packet.stream_index]->time_base;
    packet.dts = (int64_t)((dts - start) * (double)time_base.den /
                           (double)time_base.num);
    packet.pts = (int64_t)((pts - start) * (double)time_base.den /
                           (double)time_base.num);
    av_write_frame(out_ctx, &packet);
  }

  av_write_trailer(out_ctx);

  avio_close(out_ctx->pb);
  avformat_free_context(in_ctx);
  avformat_free_context(out_ctx);
  return TRUE;

fail:
  if (out_ctx->pb) {
    avio_close(out_ctx->pb);
  }
  avformat_free_context(in_ctx);
  avformat_free_context(out_ctx);
  return FALSE;
}
