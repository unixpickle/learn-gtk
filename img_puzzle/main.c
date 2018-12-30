#include <gtk/gtk.h>
#include <math.h>

#define PIECE_SIZE 64
#define NUM_PIECES 5
#define EXTRA_ROWS 2
#define GRID_SIZE (PIECE_SIZE * NUM_PIECES)
#define BUTTON_SPACE 80
#define NUM_SWAPS 100

GtkWidget* puzzle_contents = NULL;
GtkWidget* window = NULL;

GtkWidget* drag_widget = NULL;
int drag_start_x = 0;
int drag_start_y = 0;

static void close_key_press(GtkWidget* widget,
                            GdkEventKey* event,
                            gpointer userData) {
  if (event->keyval == GDK_KEY_Escape ||
      ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_w)) {
    gtk_window_close(GTK_WINDOW(widget));
  }
}

static void piece_mouse_press(GtkWidget* widget,
                              GdkEventButton* event,
                              gpointer userData) {
  drag_widget = widget;

  GtkAllocation rect;
  gtk_widget_get_allocation(widget, &rect);
  drag_start_x = rect.x;
  drag_start_y = rect.y;

  // Bring the piece to the front.
  g_object_ref(widget);
  gtk_container_remove(GTK_CONTAINER(puzzle_contents), widget);
  gtk_fixed_put(GTK_FIXED(puzzle_contents), widget, rect.x, rect.y);
  g_object_unref(widget);
}

static void piece_mouse_motion(GtkWidget* widget,
                               GdkEventMotion* event,
                               gpointer userData) {
  GtkAllocation rect;
  gtk_widget_get_allocation(widget, &rect);
  int new_x = rect.x + event->x - PIECE_SIZE / 2;
  int new_y = rect.y + event->y - PIECE_SIZE / 2;
  new_x = MAX(0, MIN(new_x, PIECE_SIZE * (NUM_PIECES - 1)));
  new_y = MAX(0, MIN(new_y, PIECE_SIZE * (NUM_PIECES - 1 + EXTRA_ROWS)));
  gtk_fixed_move(GTK_FIXED(puzzle_contents), drag_widget, new_x, new_y);
}

static void piece_mouse_release(GtkWidget* widget,
                                GdkEventButton* event,
                                gpointer userData) {
  if (!drag_widget) {
    return;
  }
  GtkAllocation rect;
  gtk_widget_get_allocation(drag_widget, &rect);
  int dest_x = (int)round((float)rect.x / PIECE_SIZE) * PIECE_SIZE;
  int dest_y = (int)round((float)rect.y / PIECE_SIZE) * PIECE_SIZE;

  GList* children = gtk_container_get_children(GTK_CONTAINER(puzzle_contents));
  for (guint i = 0; i < g_list_length(children); ++i) {
    GtkWidget* widget = (GtkWidget*)g_list_nth_data(children, i);
    gtk_widget_get_allocation(widget, &rect);
    if (rect.x == dest_x && rect.y == dest_y) {
      dest_x = drag_start_x;
      dest_y = drag_start_y;
      break;
    }
  }

  gtk_fixed_move(GTK_FIXED(puzzle_contents), drag_widget, dest_x, dest_y);

  drag_widget = NULL;
}

static void remove_existing_puzzle() {
  GList* children = gtk_container_get_children(GTK_CONTAINER(puzzle_contents));
  for (guint i = 0; i < g_list_length(children); i++) {
    gtk_container_remove(GTK_CONTAINER(puzzle_contents),
                         (GtkWidget*)g_list_nth_data(children, i));
  }
  g_list_free(children);
}

static void scramble_puzzle(GtkWidget* button, gpointer userData) {
  GList* children = gtk_container_get_children(GTK_CONTAINER(puzzle_contents));
  if (!g_list_length(children)) {
    return;
  }
  GtkAllocation* rects =
      (GtkAllocation*)malloc(sizeof(GtkAllocation) * g_list_length(children));
  for (guint i = 0; i < g_list_length(children); ++i) {
    GtkWidget* widget = (GtkWidget*)g_list_nth_data(children, i);
    gtk_widget_get_allocation(widget, &rects[i]);
  }
  for (int i = 0; i < NUM_SWAPS; ++i) {
    int idx1 = rand() % g_list_length(children);
    int idx2 = rand() % g_list_length(children);
    if (idx1 == idx2) {
      --i;
      continue;
    }
    GtkAllocation r1 = rects[idx1];
    rects[idx1] = rects[idx2];
    rects[idx2] = r1;
  }
  for (guint i = 0; i < g_list_length(children); ++i) {
    GtkWidget* widget = (GtkWidget*)g_list_nth_data(children, i);
    gtk_fixed_move(GTK_FIXED(puzzle_contents), widget, rects[i].x, rects[i].y);
  }
  g_list_free(children);
}

static GtkWidget* register_piece_events(GtkWidget* piece) {
  GtkWidget* box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(box), piece);
  g_signal_connect(box, "button_press_event", G_CALLBACK(piece_mouse_press),
                   NULL);
  g_signal_connect(box, "motion_notify_event", G_CALLBACK(piece_mouse_motion),
                   NULL);
  g_signal_connect(box, "button_release_event", G_CALLBACK(piece_mouse_release),
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

  GdkPixbuf* scaled = gdk_pixbuf_scale_simple(pixbuf, GRID_SIZE, GRID_SIZE,
                                              GDK_INTERP_BILINEAR);
  g_object_unref(pixbuf);
  for (int x = 0; x < GRID_SIZE; x += PIECE_SIZE) {
    for (int y = 0; y < GRID_SIZE; y += PIECE_SIZE) {
      GdkPixbuf* sub_buf =
          gdk_pixbuf_new_subpixbuf(scaled, x, y, PIECE_SIZE, PIECE_SIZE);
      GtkWidget* image = gtk_image_new_from_pixbuf(sub_buf);
      GtkWidget* piece = register_piece_events(image);
      gtk_fixed_put(GTK_FIXED(puzzle_contents), piece, x, y);
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
  puzzle_contents = gtk_fixed_new();
  gtk_widget_set_size_request(puzzle_contents, GRID_SIZE,
                              GRID_SIZE + PIECE_SIZE * EXTRA_ROWS);

  GtkWidget* choose_button = gtk_button_new_with_label("Choose Image...");
  g_signal_connect(choose_button, "clicked", G_CALLBACK(choose_file), NULL);

  GtkWidget* scramble_button = gtk_button_new_with_label("Scramble");
  g_signal_connect(scramble_button, "clicked", G_CALLBACK(scramble_puzzle),
                   NULL);

  GtkWidget* button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_CENTER);
  gtk_widget_set_size_request(button_box, GRID_SIZE, BUTTON_SPACE);
  gtk_container_add(GTK_CONTAINER(button_box), choose_button);
  gtk_container_add(GTK_CONTAINER(button_box), scramble_button);

  GtkWidget* root_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(root_container), puzzle_contents);
  gtk_container_add(GTK_CONTAINER(root_container), button_box);

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Image Puzzle");
  gtk_window_set_default_size(
      GTK_WINDOW(window), GRID_SIZE,
      GRID_SIZE + PIECE_SIZE * EXTRA_ROWS + BUTTON_SPACE);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK |
                                    GDK_POINTER_MOTION_MASK);
  g_signal_connect(window, "key_press_event", G_CALLBACK(close_key_press),
                   NULL);
  gtk_container_add(GTK_CONTAINER(window), root_container);
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