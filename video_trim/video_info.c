#include "video_info.h"
#include <libavformat/avformat.h>

gboolean video_duration(const char* path, double* duration) {
  av_register_all();
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
