#ifndef __MESH_H__
#define __MESH_H__

struct physics_state {
  float x;
  float y;
  float vx;
  float vy;
};

struct particle {
  struct physics_state s;
  char is_edge;

  struct physics_state _tmp_1;
  struct physics_state _tmp_2;
};

struct spring {
  struct particle* p1;
  struct particle* p2;
  float base_len;
  float k;
};

struct mesh {
  int num_particles;
  struct particle* particles;

  int num_springs;
  struct spring* springs;

  float max_vel;
  float damping;
};

float particle_distance(struct particle* p1, struct particle* p2);
float physics_distance(struct physics_state* p1, struct physics_state* p2);

struct mesh* mesh_new_grid(float spacing, float x, float y, int rows, int cols);
struct mesh* mesh_new_fc(float spacing,
                         float x,
                         float y,
                         int rows,
                         int cols,
                         float max_dist,
                         char add_edges);
struct mesh* mesh_new_edge_conn(float spacing,
                                float x,
                                float y,
                                int rows,
                                int cols);
void mesh_step(struct mesh* m, float time_frac);
void mesh_free(struct mesh* m);

#endif
