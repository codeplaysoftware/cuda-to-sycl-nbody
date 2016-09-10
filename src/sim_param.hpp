#pragma once

#include <cstdlib>

struct sim_param
{
  float G;                     ///< Gravitational parameter
  float frame_time;            ///< Minimum frame time in seconds
  float dt;                    ///< Simulation delta t
  size_t num_particles;        ///< Number of particles simulated
  int max_iterations_per_frame;///< Simulation iterations per frame rendered
  float damping;               ///< Damping parameter for simulating 'soupy' galaxy (1.0 = no damping)
};

sim_param sim_param_default();
sim_param sim_param_parse_args(sim_param p, int argc, char **argv);