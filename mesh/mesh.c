#include "mesh.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <strings.h>

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
  mesh->num_particles = rows * cols;
  mesh->particles = malloc(sizeof(struct particle) * mesh->num_particles);
  mesh->num_springs = 2 * mesh->num_particles - (rows + cols);
  mesh->springs = malloc(sizeof(struct spring) * mesh->num_springs);

  mesh->max_vel = 100;
  mesh->damping = 0.5;

  int spring_idx = 0;
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      struct particle* p = &mesh->particles[i * cols + j];
      p->x = x + (float)j * spacing;
      p->y = y + (float)i * spacing;
      p->vx = 0;
      p->vy = 0;

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