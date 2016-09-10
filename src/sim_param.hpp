#pragma once

#include <cstdlib>

/**
 * Simulation parameters
 */
struct sim_param
{
  float G;                     ///< Gravitational parameter
  float frame_time;            ///< Minimum frame time in seconds
  float dt;                    ///< Simulation delta t
  size_t num_particles;        ///< Number of particles simulated
  int max_iterations_per_frame;///< Simulation iterations per frame rendered
  float damping;               ///< Damping parameter for simulating 'soupy' galaxy (1.0 = no damping)
};

/**
 * Creates default simulation parameters
 */
sim_param sim_param_default();

/**
 * Provides user-defined simulation parameters
 * @param p parameters to be modified
 * @param argc number of arguments
 * @param argv arguments
 * @return modified simulation parameters
 */
sim_param sim_param_parse_args(sim_param p, int argc, char **argv);