#ifndef __VIDEO_INFO_H__
#define __VIDEO_INFO_H__

#include <gtk/gtk.h>

#define PROGRESS_SUCCESS ((float)2.0)
#define PROGRESS_FAILURE ((float)-1.0)

gboolean video_duration(const char* path, double* duration);
void cut_video(char* in_path,
               char* out_path,
               double start,
               double end,
               GSourceFunc progress_cb);

#endif