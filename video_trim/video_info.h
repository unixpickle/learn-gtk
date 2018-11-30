#ifndef __VIDEO_INFO_H__
#define __VIDEO_INFO_H__

#include <gtk/gtk.h>

#define PROGRESS_SUCCESS ((float)2.0)
#define PROGRESS_FAILURE ((float)-1.0)

gboolean video_duration(const char* path, double* duration);
void cut_video(char* inPath,
               char* outPath,
               double start,
               double end,
               GSourceFunc progressCb);

#endif