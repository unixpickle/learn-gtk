#include "video_info.h"
#include <libavformat/avformat.h>
#include <pthread.h>

typedef struct {
  char* inPath;
  char* outPath;
  double start;
  double end;
  GSourceFunc progressCb;
} cut_arguments_t;

static void* cut_video_thread(void* arguments);
static gboolean cut_video_internal(const char* inPath,
                                   const char* outPath,
                                   double start,
                                   double end,
                                   GSourceFunc progressCb);

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

void cut_video(char* inPath,
               char* outPath,
               double start,
               double end,
               GSourceFunc progressCb) {
  cut_arguments_t* args = (cut_arguments_t*)malloc(sizeof(cut_arguments_t));
  args->inPath = inPath;
  args->outPath = outPath;
  args->start = start;
  args->end = end;
  args->progressCb = progressCb;

  pthread_t thread;
  pthread_create(&thread, NULL, cut_video_thread, args);
  pthread_detach(thread);
}

static void* cut_video_thread(void* arguments) {
  cut_arguments_t* args = (cut_arguments_t*)arguments;
  float* value = (float*)g_malloc(sizeof(float));
  *value = PROGRESS_FAILURE;
  if (cut_video_internal(args->inPath, args->outPath, args->start, args->end,
                         args->progressCb)) {
    *value = PROGRESS_SUCCESS;
  }
  g_main_context_invoke_full(NULL, 0, args->progressCb, value, g_free);
  g_free(args->inPath);
  g_free(args->outPath);
  free(args);
  return NULL;
}

gboolean cut_video_internal(const char* inPath,
                            const char* outPath,
                            double start,
                            double end,
                            GSourceFunc progressCb) {
  AVFormatContext* inCtx = avformat_alloc_context();
  if (avformat_open_input(&inCtx, inPath, NULL, NULL) != 0) {
    return FALSE;
  }
  if (avformat_find_stream_info(inCtx, NULL) < 0) {
    avformat_free_context(inCtx);
    return FALSE;
  }

  AVFormatContext* outCtx;
  if (avformat_alloc_output_context2(&outCtx, NULL, NULL, outPath)) {
    avformat_free_context(inCtx);
    return FALSE;
  }

  for (int i = 0; i < inCtx->nb_streams; ++i) {
    AVStream* inStream = inCtx->streams[i];
    AVStream* outStream = avformat_new_stream(
        outCtx, avcodec_find_encoder(inStream->codecpar->codec_id));
    avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
    outStream->codecpar->codec_tag = 0;
  }

  avio_open(&outCtx->pb, outPath, AVIO_FLAG_WRITE);
  if (avformat_init_output(outCtx, NULL) < 0) {
    goto fail;
  }
  if (avformat_write_header(outCtx, NULL) < 0) {
    goto fail;
  }

  int64_t startTime = (int64_t)(start * (double)AV_TIME_BASE);
  if (av_seek_frame(inCtx, -1, startTime, 0) < 0) {
    goto fail;
  }
  AVPacket packet;
  av_init_packet(&packet);
  while (av_read_frame(inCtx, &packet) >= 0) {
    AVRational timeBase = inCtx->streams[packet.stream_index]->time_base;
    double dts =
        ((double)packet.dts * (double)timeBase.num) / (double)timeBase.den;
    double pts =
        ((double)packet.pts * (double)timeBase.num) / (double)timeBase.den;
    if (dts > end) {
      break;
    }
    timeBase = outCtx->streams[packet.stream_index]->time_base;
    packet.dts =
        (int64_t)((dts - start) * (double)timeBase.den / (double)timeBase.num);
    if (packet.pts != AV_NOPTS_VALUE) {
      packet.pts = (int64_t)((pts - start) * (double)timeBase.den /
                             (double)timeBase.num);
    }
    av_write_frame(outCtx, &packet);
  }

  av_write_trailer(outCtx);

  avio_close(outCtx->pb);
  avformat_free_context(inCtx);
  avformat_free_context(outCtx);
  return TRUE;

fail:
  if (outCtx->pb) {
    avio_close(outCtx->pb);
  }
  avformat_free_context(inCtx);
  avformat_free_context(outCtx);
  return FALSE;
}
