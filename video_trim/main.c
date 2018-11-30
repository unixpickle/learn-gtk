#include <gtk/gtk.h>
#include "video_info.h"

static GtkWidget* fileChooser;
static GtkWidget* startScale;
static GtkWidget* endScale;
static GtkWidget* timesGrid;

static void activate(GtkApplication* app, gpointer userData);
static void handle_key_event(GtkWidget* widget,
                             GdkEventKey* event,
                             gpointer userData);
static void handle_file_set(GtkWidget* widget, gpointer userData);
static void toggle_controls(gboolean enabled);

int main(int argc, char** argv) {
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

  GtkWidget* rootContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_top(rootContainer, 10);
  gtk_widget_set_margin_bottom(rootContainer, 10);
  gtk_widget_set_margin_start(rootContainer, 10);
  gtk_widget_set_margin_end(rootContainer, 10);
  gtk_box_set_spacing(GTK_BOX(rootContainer), 10);
  gtk_container_add(GTK_CONTAINER(rootContainer), fileChooser);
  gtk_container_add(GTK_CONTAINER(rootContainer), timesGrid);

  GtkWidget* window = gtk_application_window_new(app);
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

static void toggle_controls(gboolean enabled) {
  gtk_widget_set_sensitive(timesGrid, enabled);
}
