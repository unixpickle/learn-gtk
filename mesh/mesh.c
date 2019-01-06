#include "mesh.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <strings.h>

#define MAX_VEL 1000
#define DAMPING 0.5

static struct spring* add_spring(struct mesh* mesh) {
  mesh->springs =
      realloc(mesh->springs, (mesh->num_springs + 1) * sizeof(struct spring));
  return &mesh->springs[mesh->num_springs++];
}

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
      p->s.x = x + (float)j * spacing;
      p->s.y = y + (float)i * spacing;
      p->s.vx = 0;
      p->s.vy = 0;
      p->is_edge = i == 0 || i == rows - 1 || j == 0 || j == cols - 1;
    }
  }
}

static void add_grid_springs(struct mesh* mesh, int rows, int cols) {
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      struct particle* p = &mesh->particles[i * cols + j];

#define ADD_SPRING                        \
  struct spring* s = add_spring(mesh);    \
  s->p1 = p1;                             \
  s->p2 = p;                              \
  s->base_len = particle_distance(p, p1); \
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
}

static void add_fc_springs(struct mesh* mesh, float max_dist) {
  for (int i = 0; i < mesh->num_particles; ++i) {
    struct particle* p = &mesh->particles[i];
    for (int j = 0; j < i; ++j) {
      struct particle* p1 = &mesh->particles[j];
      float d = particle_distance(p, p1);
      if (d <= max_dist) {
        struct spring* s = add_spring(mesh);
        s->p1 = p1;
        s->p2 = p;
        s->base_len = particle_distance(p, p1);
        s->k = 10.0 / s->base_len;
      }
    }
  }
}

static void add_edge_conn_springs(struct mesh* mesh) {
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
      struct spring* s = add_spring(mesh);
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

static struct physics_state* _get_phys_state(struct particle* p, int idx) {
  if (idx == 0) {
    return &p->s;
  } else if (idx == 1) {
    return &p->_tmp_1;
  } else if (idx == 2) {
    return &p->_tmp_2;
  }
  return NULL;
}

static void _mesh_step(struct mesh* m, float time_frac, int substep) {
  for (int i = 0; i < m->num_particles; ++i) {
    struct particle* p = &m->particles[i];
    *_get_phys_state(p, substep + 1) = *_get_phys_state(p, substep);
  }

  for (int i = 0; i < m->num_springs; ++i) {
    struct spring* s = &m->springs[i];
    struct physics_state* p1 = _get_phys_state(s->p1, substep + 1);
    struct physics_state* p2 = _get_phys_state(s->p2, substep + 1);
    float dist = physics_distance(p1, p2);
    float force = s->k * (dist - s->base_len);
    p1->vx += time_frac * force * (p2->x - p1->x);
    p1->vy += time_frac * force * (p2->y - p1->y);
    p2->vx -= time_frac * force * (p2->x - p1->x);
    p2->vy -= time_frac * force * (p2->y - p1->y);
  }

  for (int i = 0; i < m->num_particles; ++i) {
    struct particle* p = &m->particles[i];

    struct physics_state* p_old = _get_phys_state(p, substep);
    struct physics_state* p_new = _get_phys_state(p, substep + 1);

    float vdamp = pow(m->damping, time_frac);
    p_new->vx *= vdamp;
    p_new->vy *= vdamp;

    float vmag = mag(p_new->vx, p_new->vy);
    if (vmag > m->max_vel) {
      float scale = m->max_vel / vmag;
      p_new->vx *= scale;
      p_new->vy *= scale;
    }

    p_new->x += time_frac * p_old->vx;
    p_new->y += time_frac * p_old->vy;
  }
}

static void _mesh_step_final(struct mesh* m) {
  for (int i = 0; i < m->num_particles; ++i) {
    struct particle* p = &m->particles[i];

    // The RK2 algorithm, which ends up being unstable:
    // p->s.x += 0.5 * ((p->_tmp_2.x - p->_tmp_1.x) + (p->_tmp_1.x - p->s.x));
    // p->s.y += 0.5 * ((p->_tmp_2.y - p->_tmp_1.y) + (p->_tmp_1.y - p->s.y));
    // p->s.vx += 0.5 * ((p->_tmp_2.vx - p->_tmp_1.vx) + (p->_tmp_1.vx -
    // p->s.vx)); p->s.vy += 0.5 * ((p->_tmp_2.vy - p->_tmp_1.vy) +
    // (p->_tmp_1.vy - p->s.vy));

    // The forward Euler algorithm, which is fairly ustable:
    // p->s = p->_tmp_1;

    // The backward Euler algorithm, which is stable:
    p->s.x += p->_tmp_2.x - p->_tmp_1.x;
    p->s.y += p->_tmp_2.y - p->_tmp_1.y;
    p->s.vx += p->_tmp_2.vx - p->_tmp_1.vx;
    p->s.vy += p->_tmp_2.vy - p->_tmp_1.vy;
  }
}

float physics_distance(struct physics_state* p1, struct physics_state* p2) {
  return mag(p1->x - p2->x, p1->y - p2->y);
}

float particle_distance(struct particle* p1, struct particle* p2) {
  return physics_distance(&p1->s, &p2->s);
}

struct mesh* mesh_new_grid(float spacing,
                           float x,
                           float y,
                           int rows,
                           int cols) {
  struct mesh* mesh = malloc(sizeof(struct mesh));
  bzero(mesh, sizeof(struct mesh));
  mesh->max_vel = MAX_VEL;
  mesh->damping = DAMPING;
  add_grid_particles(mesh, spacing, x, y, rows, cols);
  add_grid_springs(mesh, rows, cols);
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
  mesh->max_vel = MAX_VEL;
  mesh->damping = DAMPING;
  add_grid_particles(mesh, spacing, x, y, rows, cols);
  add_fc_springs(mesh, max_dist);
  if (add_edges) {
    add_edge_conn_springs(mesh);
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
  mesh->max_vel = MAX_VEL;
  mesh->damping = DAMPING;
  add_grid_particles(mesh, spacing, x, y, rows, cols);
  add_edge_conn_springs(mesh);
  return mesh;
}

void mesh_step(struct mesh* m, float time_frac) {
  _mesh_step(m, time_frac, 0);
  _mesh_step(m, time_frac, 1);
  _mesh_step_final(m);
}

void mesh_free(struct mesh* m) {
  free(m->particles);
  free(m->springs);
  free(m);
}
