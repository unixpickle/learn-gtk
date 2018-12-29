#include "mesh.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <strings.h>

static void add_grid_particles(struct mesh* mesh,
                               float spacing,
                               float x,
                               float y,
                               int rows,
                               int cols) {
  mesh->num_particles = rows * cols;
  mesh->particles = malloc(sizeof(struct particle) * mesh->num_particles);
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      struct particle* p = &mesh->particles[i * cols + j];
      p->x = x + (float)j * spacing;
      p->y = y + (float)i * spacing;
      p->vx = 0;
      p->vy = 0;
      p->is_edge = i == 0 || i == rows - 1 || j == 0 || j == cols - 1;
    }
  }
}

static void add_fully_conns(struct mesh* mesh, float max_dist) {
  for (int i = 0; i < mesh->num_particles; ++i) {
    struct particle* p = &mesh->particles[i];
    for (int j = 0; j < i; ++j) {
      struct particle* p1 = &mesh->particles[j];
      float d = particle_distance(p, p1);
      if (d <= max_dist) {
        struct spring* s = &mesh->springs[mesh->num_springs++];
        s->p1 = p1;
        s->p2 = p;
        s->base_len = particle_distance(p, p1);
        s->k = 10.0 / s->base_len;
      }
    }
  }
}

static void add_edge_conns(struct mesh* mesh) {
  for (int i = 0; i < mesh->num_particles; ++i) {
    struct particle* p1 = &mesh->particles[i];
    if (!p1->is_edge) {
      continue;
    }
    for (int j = 0; j < mesh->num_particles; ++j) {
      if (i == j) {
        continue;
      }
      struct particle* p2 = &mesh->particles[j];
      struct spring* s = &mesh->springs[mesh->num_springs++];
      s->p1 = p1;
      s->p2 = p2;
      s->base_len = particle_distance(p1, p2);
      // If we don't square base_len, the square moves
      // pretty much all at once without deforming.
      s->k = 100.0 / (s->base_len * s->base_len);
    }
  }
}

static float mag(float x, float y) {
  return sqrt(x * x + y * y);
}

float particle_distance(struct particle* p1, struct particle* p2) {
  return mag(p1->x - p2->x, p1->y - p2->y);
}

struct mesh* mesh_new_grid(float spacing,
                           float x,
                           float y,
                           int rows,
                           int cols) {
  struct mesh* mesh = malloc(sizeof(struct mesh));
  bzero(mesh, sizeof(struct mesh));
  add_grid_particles(mesh, spacing, x, y, rows, cols);
  mesh->num_springs = 2 * mesh->num_particles - (rows + cols);
  mesh->springs = malloc(sizeof(struct spring) * mesh->num_springs);
  mesh->max_vel = 100;
  mesh->damping = 0.5;

  int spring_idx = 0;
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      struct particle* p = &mesh->particles[i * cols + j];

#define ADD_SPRING                                 \
  struct spring* s = &mesh->springs[spring_idx++]; \
  s->p1 = p1;                                      \
  s->p2 = p;                                       \
  s->base_len = particle_distance(p, p1);          \
  s->k = 1.0

      if (j > 0) {
        struct particle* p1 = &mesh->particles[i * cols + j - 1];
        ADD_SPRING;
      }
      if (i > 0) {
        struct particle* p1 = &mesh->particles[(i - 1) * cols + j];
        ADD_SPRING;
      }
    }
  }
  assert(spring_idx == mesh->num_springs);

  return mesh;
}

struct mesh* mesh_new_fc(float spacing,
                         float x,
                         float y,
                         int rows,
                         int cols,
                         float max_dist,
                         char add_edges) {
  struct mesh* mesh = malloc(sizeof(struct mesh));
  bzero(mesh, sizeof(struct mesh));
  add_grid_particles(mesh, spacing, x, y, rows, cols);
  mesh->num_springs = 0;
  mesh->springs = malloc(sizeof(struct spring) * mesh->num_particles *
                         mesh->num_particles * 2);
  mesh->max_vel = 200;
  mesh->damping = 0.5;
  add_fully_conns(mesh, max_dist);
  if (add_edges) {
    add_edge_conns(mesh);
  }
  return mesh;
}

struct mesh* mesh_new_edge_conn(float spacing,
                                float x,
                                float y,
                                int rows,
                                int cols) {
  struct mesh* mesh = malloc(sizeof(struct mesh));
  bzero(mesh, sizeof(struct mesh));
  add_grid_particles(mesh, spacing, x, y, rows, cols);
  mesh->num_springs = 0;
  mesh->springs = malloc(sizeof(struct spring) *
                         ((rows * 2 + cols * 2 - 4) * mesh->num_particles));
  mesh->max_vel = 500;
  mesh->damping = 0.5;
  add_edge_conns(mesh);
  return mesh;
}

void mesh_step(struct mesh* m, float time_frac) {
  for (int i = 0; i < m->num_springs; ++i) {
    struct spring* s = &m->springs[i];
    float dist = particle_distance(s->p1, s->p2);
    float force = s->k * (dist - s->base_len);
    s->p1->vx += time_frac * force * (s->p2->x - s->p1->x);
    s->p1->vy += time_frac * force * (s->p2->y - s->p1->y);
    s->p2->vx -= time_frac * force * (s->p2->x - s->p1->x);
    s->p2->vy -= time_frac * force * (s->p2->y - s->p1->y);
  }

  for (int i = 0; i < m->num_particles; ++i) {
    struct particle* p = &m->particles[i];

    float vdamp = pow(m->damping, time_frac);
    p->vx *= vdamp;
    p->vy *= vdamp;

    float vmag = mag(p->vx, p->vy);
    if (vmag > m->max_vel) {
      float scale = m->max_vel / vmag;
      p->vx *= scale;
      p->vy *= scale;
    }

    p->x += time_frac * p->vx;
    p->y += time_frac * p->vy;
  }
}

void mesh_free(struct mesh* m) {
  free(m->particles);
  free(m->springs);
  free(m);
}
