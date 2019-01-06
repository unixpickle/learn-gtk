#include <gtk/gtk.h>
#include <libavformat/avformat.h>
#include "video_info.h"

static GtkWidget* file_chooser;
static GtkWidget* start_scale;
static GtkWidget* end_scale;
static GtkWidget* times_grid;
static GtkWidget* trim_button;
static GtkWidget* progress_bar;
static GtkWidget* window;

static void activate(GtkApplication* app, gpointer user_data);
static void handle_key_event(GtkWidget* widget,
                             GdkEventKey* event,
                             gpointer user_data);
static void handle_file_set(GtkWidget* widget, gpointer user_data);
static void handle_trim_clicked(GtkWidget* widget, gpointer user_data);
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

static void activate(GtkApplication* app, gpointer user_data) {
  file_chooser =
      gtk_file_chooser_button_new("Video File", GTK_FILE_CHOOSER_ACTION_OPEN);
  g_signal_connect(file_chooser, "file_set", G_CALLBACK(handle_file_set), NULL);

  GtkWidget* start_label = gtk_label_new("Start:");
  GtkWidget* end_label = gtk_label_new("End:");
  start_scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
  end_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01);
  gtk_widget_set_hexpand(start_scale, TRUE);
  gtk_widget_set_hexpand(end_scale, TRUE);

  times_grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(times_grid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(times_grid), 5);
  gtk_grid_attach(GTK_GRID(times_grid), start_label, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(times_grid), end_label, 0, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(times_grid), start_scale, 1, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(times_grid), end_scale, 1, 1, 1, 1);
  gtk_widget_set_sensitive(times_grid, FALSE);

  trim_button = gtk_button_new_with_label("Trim Video");
  g_signal_connect(trim_button, "clicked", G_CALLBACK(handle_trim_clicked),
                   NULL);
  gtk_widget_set_sensitive(trim_button, FALSE);

  progress_bar = gtk_progress_bar_new();

  GtkWidget* root_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_top(root_container, 10);
  gtk_widget_set_margin_bottom(root_container, 10);
  gtk_widget_set_margin_start(root_container, 10);
  gtk_widget_set_margin_end(root_container, 10);
  gtk_box_set_spacing(GTK_BOX(root_container), 10);
  gtk_container_add(GTK_CONTAINER(root_container), file_chooser);
  gtk_container_add(GTK_CONTAINER(root_container), times_grid);
  gtk_container_add(GTK_CONTAINER(root_container), trim_button);
  gtk_container_add(GTK_CONTAINER(root_container), progress_bar);

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Video Trim");
  gtk_window_set_default_size(GTK_WINDOW(window), 500, 100);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK |
                                    GDK_POINTER_MOTION_MASK);
  g_signal_connect(window, "key_press_event", G_CALLBACK(handle_key_event),
                   NULL);
  gtk_container_add(GTK_CONTAINER(window), root_container);
  gtk_widget_show_all(window);
}

static void handle_key_event(GtkWidget* widget,
                             GdkEventKey* event,
                             gpointer user_data) {
  if (event->keyval == GDK_KEY_Escape ||
      ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_w)) {
    gtk_window_close(GTK_WINDOW(widget));
  }
}

static void handle_file_set(GtkWidget* widget, gpointer user_data) {
  double duration;
  gchar* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
  if (video_duration(path, &duration)) {
    toggle_controls(TRUE);
    gtk_range_set_range(GTK_RANGE(start_scale), 0, duration);
    gtk_range_set_range(GTK_RANGE(end_scale), 0, duration);
  } else {
    toggle_controls(FALSE);
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(window), 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
        "Failed to get video length.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }
  g_free(path);
}

static void handle_trim_clicked(GtkWidget* widget, gpointer user_data) {
  char* input_name =
      gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
  int dot_index = 0;
  for (int i = 0; i < strlen(input_name); ++i) {
    if (input_name[i] == '.') {
      dot_index = i;
    }
  }
  char outputName[512];
  snprintf(outputName, sizeof(outputName), "Cut Media%s",
           &input_name[dot_index]);
  g_free(input_name);

  GtkWidget* dialog = gtk_file_chooser_dialog_new(
      "Choose Output Image", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE,
      "_Cancel", GTK_RESPONSE_CANCEL, "_Export", GTK_RESPONSE_ACCEPT, NULL);
  gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), outputName);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    toggle_controls(FALSE);
    gtk_widget_set_sensitive(file_chooser, FALSE);
    cut_video(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser)),
              gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)),
              (double)gtk_range_get_value(GTK_RANGE(start_scale)),
              (double)gtk_range_get_value(GTK_RANGE(end_scale)),
              cut_video_callback);
  }

  gtk_widget_destroy(dialog);
}

static void toggle_controls(gboolean enabled) {
  gtk_widget_set_sensitive(times_grid, enabled);
  gtk_widget_set_sensitive(trim_button, enabled);
}

static gboolean cut_video_callback(gpointer progressPtr) {
  float progress = *(float*)progressPtr;
  if (progress == PROGRESS_FAILURE || progress == PROGRESS_SUCCESS) {
    toggle_controls(TRUE);
    gtk_widget_set_sensitive(file_chooser, TRUE);
    if (progress == PROGRESS_FAILURE) {
      GtkWidget* dialog =
          gtk_message_dialog_new(GTK_WINDOW(window), 0, GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE, "Failed to trim video.");
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
    }
  } else {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar),
                                  (gdouble)progress);
  }
  return FALSE;
}
