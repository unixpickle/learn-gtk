#include <gtk/gtk.h>

GtkWidget* button_box = NULL;
GtkWidget* fixed = NULL;
GtkWidget* window = NULL;

static void randomize_button() {
  GtkAllocation but_rect;
  gtk_widget_get_clip(button_box, &but_rect);
  gint win_width, win_height;
  gtk_window_get_size(GTK_WINDOW(window), &win_width, &win_height);

  gint x = rand() % (win_width - but_rect.width);
  gint y = rand() % (win_height - but_rect.height);
  gtk_fixed_move(GTK_FIXED(fixed), button_box, x, y);
}

static void on_key_press(GtkWidget* widget,
                         GdkEventKey* event,
                         gpointer userData) {
  if (event->keyval == GDK_KEY_Escape ||
      ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_w)) {
    gtk_window_close(GTK_WINDOW(widget));
  }
}

static void on_button_press(GtkWidget* widget, gpointer userData) {
  randomize_button();
}

static void activate(GtkApplication* app, gpointer userData) {
  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Window");
  gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(window, "key_press_event", G_CALLBACK(on_key_press), NULL);

  GtkWidget* button = gtk_button_new_with_label("Click Me");
  g_signal_connect(button, "clicked", G_CALLBACK(on_button_press), NULL);

  button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_container_add(GTK_CONTAINER(button_box), button);

  fixed = gtk_fixed_new();
  gtk_fixed_put(GTK_FIXED(fixed), button_box, 0, 0);
  randomize_button();
  gtk_container_add(GTK_CONTAINER(window), fixed);

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