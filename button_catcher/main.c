#include <gtk/gtk.h>

static void on_key_press(GtkWidget* widget,
                         GdkEventKey* event,
                         gpointer userData) {
  if (event->keyval == GDK_KEY_Escape ||
      ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_w)) {
    gtk_window_close(GTK_WINDOW(widget));
  }
}

static void activate(GtkApplication* app, gpointer userData) {
  GtkWidget* window = gtk_application_window_new(app);

  gtk_window_set_title(GTK_WINDOW(window), "Window");
  gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(window, "key_press_event", G_CALLBACK(on_key_press), NULL);

  gtk_widget_show_all(window);
}

int main(int argc, char** argv) {
  GtkApplication* app = gtk_application_new("com.aqnichol.button_catcher",
                                            G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}