#include <gtk/gtk.h>
#include <libavformat/avformat.h>
#include "video_info.h"

static GtkWidget* fileChooser;
static GtkWidget* startScale;
static GtkWidget* endScale;
static GtkWidget* timesGrid;
static GtkWidget* trimButton;
static GtkWidget* progressBar;
static GtkWidget* window;

static void activate(GtkApplication* app, gpointer userData);
static void handle_key_event(GtkWidget* widget,
                             GdkEventKey* event,
                             gpointer userData);
static void handle_file_set(GtkWidget* widget, gpointer userData);
static void handle_trim_clicked(GtkWidget* widget, gpointer userData);
static void toggle_controls(gboolean enabled);
static gboolean cut_video_callback(gpointer progressPtr);

int main(int argc, char** argv) {
  av_register_all();
  GtkApplication* app =
      gtk_application_new("com.aqnichol.video_trim", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}

static void activate(GtkApplication* app, gpointer userData) {
  fileChooser =
      gtk_file_chooser_button_new("Video File", GTK_FILE_CHOOSER_ACTION_OPEN);
  g_signal_connect(fileChooser, "file_set", G_CALLBACK(handle_file_set), NULL);

  GtkWidget* startLabel = gtk_label_new("Start:");
  GtkWidget* endLabel = gtk_label_new("End:");
  startScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
  endScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
  gtk_widget_set_hexpand(startScale, TRUE);
  gtk_widget_set_hexpand(endScale, TRUE);

  timesGrid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(timesGrid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(timesGrid), 5);
  gtk_grid_attach(GTK_GRID(timesGrid), startLabel, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(timesGrid), endLabel, 0, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(timesGrid), startScale, 1, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(timesGrid), endScale, 1, 1, 1, 1);
  gtk_widget_set_sensitive(timesGrid, FALSE);

  trimButton = gtk_button_new_with_label("Trim Video");
  g_signal_connect(trimButton, "clicked", G_CALLBACK(handle_trim_clicked),
                   NULL);
  progressBar = gtk_progress_bar_new();

  GtkWidget* rootContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_top(rootContainer, 10);
  gtk_widget_set_margin_bottom(rootContainer, 10);
  gtk_widget_set_margin_start(rootContainer, 10);
  gtk_widget_set_margin_end(rootContainer, 10);
  gtk_box_set_spacing(GTK_BOX(rootContainer), 10);
  gtk_container_add(GTK_CONTAINER(rootContainer), fileChooser);
  gtk_container_add(GTK_CONTAINER(rootContainer), timesGrid);
  gtk_container_add(GTK_CONTAINER(rootContainer), trimButton);
  gtk_container_add(GTK_CONTAINER(rootContainer), progressBar);

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Video Trim");
  gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK |
                                    GDK_POINTER_MOTION_MASK);
  g_signal_connect(window, "key_press_event", G_CALLBACK(handle_key_event),
                   NULL);
  gtk_container_add(GTK_CONTAINER(window), rootContainer);
  gtk_widget_show_all(window);
}

static void handle_key_event(GtkWidget* widget,
                             GdkEventKey* event,
                             gpointer userData) {
  if (event->keyval == GDK_KEY_Escape ||
      ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_w)) {
    gtk_window_close(GTK_WINDOW(widget));
  }
}

static void handle_file_set(GtkWidget* widget, gpointer userData) {
  double duration;
  gchar* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
  if (video_duration(path, &duration)) {
    toggle_controls(TRUE);
    gtk_range_set_range(GTK_RANGE(startScale), 0, duration);
    gtk_range_set_range(GTK_RANGE(endScale), 0, duration);
  } else {
    toggle_controls(FALSE);
  }
  g_free(path);
}

static void handle_trim_clicked(GtkWidget* widget, gpointer userData) {
  char* inputName =
      gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileChooser));
  int dotIndex = 0;
  for (int i = 0; i < strlen(inputName); ++i) {
    if (inputName[i] == '.') {
      dotIndex = i;
    }
  }
  char outputName[512];
  snprintf(outputName, sizeof(outputName), "Cut Media%s", &inputName[dotIndex]);
  g_free(inputName);

  GtkWidget* dialog = gtk_file_chooser_dialog_new(
      "Choose Output Image", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE,
      "_Cancel", GTK_RESPONSE_CANCEL, "_Export", GTK_RESPONSE_ACCEPT, NULL);
  gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), outputName);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    toggle_controls(FALSE);
    gtk_widget_set_sensitive(fileChooser, FALSE);
    cut_video(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileChooser)),
              gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)),
              (double)gtk_range_get_value(GTK_RANGE(startScale)),
              (double)gtk_range_get_value(GTK_RANGE(endScale)),
              cut_video_callback);
  }

  gtk_widget_destroy(dialog);
}

static void toggle_controls(gboolean enabled) {
  gtk_widget_set_sensitive(timesGrid, enabled);
  gtk_widget_set_sensitive(trimButton, enabled);
}

static gboolean cut_video_callback(gpointer progressPtr) {
  float progress = *(float*)progressPtr;
  if (progress == PROGRESS_FAILURE || progress == PROGRESS_SUCCESS) {
    toggle_controls(TRUE);
    gtk_widget_set_sensitive(fileChooser, TRUE);
    if (progress == PROGRESS_FAILURE) {
      printf("error!\n");
      // TODO: show error here.
    }
  } else {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar),
                                  (gdouble)progress);
  }
  return FALSE;
}
