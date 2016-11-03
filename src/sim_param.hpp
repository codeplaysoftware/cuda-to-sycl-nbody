#pragma once

#include <cstdlib>

/**
 * Simulation parameters
 */
class sim_param
{
public:
	/**
	 * Creates default simulation parameters
 	 */
	sim_param();

	/**
	 * Provides user-defined simulation parameters
	 * @param p parameters to be modified
	 *  @param argc number of arguments
	 * @param argv arguments
	 * @return modified simulation parameters
	 */
	void parse_args(int argc, char **argv);
	
  float G;                     ///< Gravitational parameter
  float frame_time;            ///< Minimum frame time in seconds
  float dt;                    ///< Simulation delta t
  size_t num_particles;        ///< Number of particles simulated
  int max_iterations_per_frame;///< Simulation iterations per frame rendered
  float damping;               ///< Damping parameter for simulating 'soupy' galaxy (1.0 = no damping)
};