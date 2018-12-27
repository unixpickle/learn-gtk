#include <gtk/gtk.h>
#include <math.h>
#include "mesh.h"

GtkWidget* window = NULL;
GtkWidget* drawing_area = NULL;
cairo_surface_t* surface = NULL;
struct mesh* mesh = NULL;
struct particle* dragging_particle = NULL;
float drag_x = 0;
float drag_y = 0;

static void fill_white() {
  cairo_t* c = cairo_create(surface);
  cairo_set_source_rgb(c, 1, 1, 1);
  cairo_paint(c);
  cairo_destroy(c);
}

static gboolean drawing_area_configure(GtkWidget* widget,
                                       GdkEventConfigure* event,
                                       gpointer data) {
  if (surface) {
    cairo_surface_destroy(surface);
  }
  surface = gdk_window_create_similar_surface(
      gtk_widget_get_window(widget), CAIRO_CONTENT_COLOR,
      gtk_widget_get_allocated_width(widget),
      gtk_widget_get_allocated_height(widget));
  fill_white();
  return TRUE;
}

static gboolean drawing_area_draw(GtkWidget* widget,
                                  cairo_t* c,
                                  gpointer data) {
  cairo_set_source_surface(c, surface, 0, 0);
  cairo_set_source_rgb(c, 1, 1, 1);
  cairo_paint(c);

  cairo_set_source_rgb(c, 0.5, 0.5, 0.5);
  for (int i = 0; i < mesh->num_springs; ++i) {
    struct spring* s = &mesh->springs[i];
    cairo_new_sub_path(c);
    cairo_move_to(c, s->p1->x, s->p1->y);
    cairo_line_to(c, s->p2->x, s->p2->y);
    cairo_stroke(c);
  }

  cairo_set_source_rgb(c, 0, 0, 0);
  for (int i = 0; i < mesh->num_particles; ++i) {
    struct particle* p = &mesh->particles[i];
    cairo_new_sub_path(c);
    cairo_arc(c, p->x, p->y, 5, 0, (float)M_PI * 2.0f);
    cairo_close_path(c);
    cairo_fill(c);
  }

  return FALSE;
}

static gboolean perform_step(gpointer data) {
  mesh_step(mesh, 1.0 / 24.0);
  if (dragging_particle) {
    dragging_particle->x = drag_x;
    dragging_particle->y = drag_y;
  }
  gtk_widget_queue_draw(drawing_area);
  return TRUE;
}

static gboolean mouse_pressed(GtkWidget* widget,
                              GdkEventButton* event,
                              gpointer data) {
  struct particle p;
  p.x = event->x;
  p.y = event->y;
  float distance = 1000000.0f;
  for (int i = 0; i < mesh->num_particles; ++i) {
    struct particle* other = &mesh->particles[i];
    float d = particle_distance(&p, other);
    if (d < distance) {
      distance = d;
      dragging_particle = other;
    }
  }
  drag_x = event->x;
  drag_y = event->y;
  return TRUE;
}

static gboolean mouse_released(GtkWidget* widget,
                               GdkEventButton* event,
                               gpointer data) {
  dragging_particle = NULL;
}

static gboolean mouse_moved(GtkWidget* widget,
                            GdkEventMotion* event,
                            gpointer data) {
  if (event->state & GDK_BUTTON1_MASK) {
    drag_x = event->x;
    drag_y = event->y;
  }
  return TRUE;
}

static void activate(GtkApplication* app, gpointer userData) {
  mesh = mesh_new_grid(30.0f, 20.0f, 20.0f, 13, 13);

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Mesh");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);

  drawing_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area, 400, 400);
  gtk_container_add(GTK_CONTAINER(window), drawing_area);

  g_signal_connect(drawing_area, "draw", G_CALLBACK(drawing_area_draw), NULL);
  g_signal_connect(drawing_area, "configure-event",
                   G_CALLBACK(drawing_area_configure), NULL);
  g_signal_connect(drawing_area, "button-press-event",
                   G_CALLBACK(mouse_pressed), NULL);
  g_signal_connect(drawing_area, "button-release-event",
                   G_CALLBACK(mouse_released), NULL);
  g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(mouse_moved),
                   NULL);

  gtk_widget_set_events(drawing_area, gtk_widget_get_events(drawing_area) |
                                          GDK_BUTTON_PRESS_MASK |
                                          GDK_BUTTON_RELEASE_MASK |
                                          GDK_POINTER_MOTION_MASK);

  gtk_widget_show_all(window);

  gdk_threads_add_timeout(42, perform_step, NULL);
}

int main(int argc, char** argv) {
  GtkApplication* app =
      gtk_application_new("com.aqnichol.mesh", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
