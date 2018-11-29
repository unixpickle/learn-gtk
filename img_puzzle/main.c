#include <gtk/gtk.h>

GtkWidget* puzzleContents = NULL;
GtkWidget* window = NULL;

int dragStartX = 0;
int dragStartY = 0;
int dragStartPieceX = 0;
int dragStartPieceY = 0;

static void close_key_press(GtkWidget* widget,
                            GdkEventKey* event,
                            gpointer userData) {
  if (event->keyval == GDK_KEY_Escape ||
      ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_w)) {
    gtk_window_close(GTK_WINDOW(widget));
  }
}

static void piece_mouse_button(GtkWidget* widget,
                               GdkEventButton* event,
                               gpointer userData) {
  // TODO: this.
  printf("button event\n");
}

static void piece_mouse_motion(GtkWidget* widget,
                               GdkEventMotion* event,
                               gpointer userData) {
  // TODO: this.
  printf("motion event\n");
}

static void remove_existing_puzzle() {
  GList* children = gtk_container_get_children(GTK_CONTAINER(puzzleContents));
  for (guint i = 0; i < g_list_length(children); i++) {
    gtk_container_remove(GTK_CONTAINER(puzzleContents),
                         (GtkWidget*)g_list_nth_data(children, i));
  }
  g_list_free(children);
}

static GtkWidget* register_piece_events(GtkWidget* piece) {
  GtkWidget* box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(box), piece);
  g_signal_connect(box, "motion_notify_event", G_CALLBACK(piece_mouse_motion),
                   NULL);
  g_signal_connect(box, "button_press_event", G_CALLBACK(piece_mouse_button),
                   NULL);
  return box;
}

static void open_file(const char* filename) {
  GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
  if (!pixbuf) {
    GtkWidget* dialog =
        gtk_message_dialog_new(GTK_WINDOW(window), 0, GTK_MESSAGE_ERROR,
                               GTK_BUTTONS_CLOSE, "Error reading image.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return;
  }

  remove_existing_puzzle();

  GdkPixbuf* scaled =
      gdk_pixbuf_scale_simple(pixbuf, 320, 320, GDK_INTERP_BILINEAR);
  g_object_unref(pixbuf);
  for (int x = 0; x < 320; x += 320 / 5) {
    for (int y = 0; y < 320; y += 320 / 5) {
      GdkPixbuf* subBuf =
          gdk_pixbuf_new_subpixbuf(scaled, x, y, 320 / 5, 320 / 5);
      GtkWidget* image = gtk_image_new_from_pixbuf(subBuf);
      GtkWidget* piece = register_piece_events(image);
      gtk_fixed_put(GTK_FIXED(puzzleContents), piece, x, y);
      gtk_widget_show(image);
      gtk_widget_show(piece);
    }
  }
  g_object_unref(scaled);
}

static void choose_file(GtkWidget* button, gpointer userData) {
  GtkWidget* dialog = gtk_file_chooser_dialog_new(
      "Open Image", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel",
      GTK_RESPONSE_CANCEL, "_Choose", GTK_RESPONSE_ACCEPT, NULL);

  gchar* filename = NULL;
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
  }
  gtk_widget_destroy(dialog);
  if (!filename) {
    return;
  }

  open_file(filename);
  g_free(filename);
}

static void activate(GtkApplication* app, gpointer userData) {
  puzzleContents = gtk_fixed_new();
  gtk_widget_set_size_request(puzzleContents, 320, 400);

  GtkWidget* chooseButton = gtk_button_new_with_label("Choose Image...");
  g_signal_connect(chooseButton, "clicked", G_CALLBACK(choose_file), NULL);

  GtkWidget* buttonBox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonBox), GTK_BUTTONBOX_CENTER);
  gtk_widget_set_size_request(buttonBox, 320, 480 - 400);
  gtk_container_add(GTK_CONTAINER(buttonBox), chooseButton);

  GtkWidget* rootContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(rootContainer), puzzleContents);
  gtk_container_add(GTK_CONTAINER(rootContainer), buttonBox);

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Image Puzzle");
  gtk_window_set_default_size(GTK_WINDOW(window), 320, 480);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK |
                                    GDK_POINTER_MOTION_MASK);
  g_signal_connect(window, "key_press_event", G_CALLBACK(close_key_press),
                   NULL);
  gtk_container_add(GTK_CONTAINER(window), rootContainer);
  gtk_widget_show_all(window);
}

int main(int argc, char** argv) {
  GtkApplication* app =
      gtk_application_new("com.aqnichol.img_puzzle", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}