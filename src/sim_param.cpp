#include "sim_param.hpp"

#include <cstdlib>

sim_param::sim_param()
{
  G = 2.0;
  frame_time = 1.0/60.0; // 60 fps
  dt = 0.005;
  num_particles = 50*256;
  max_iterations_per_frame = 4;
  damping = 0.999998;
}

void sim_param::parse_args(int argc, char **argv)
{
  // First argument if existing = number of particle batches (256 per batch)
  if (argc >= 2)
    num_particles = 256*atoi(argv[1]);

  // Second argument if existing = number of iterations per frame
  if (argc >= 3)
    max_iterations_per_frame = atoi(argv[2]);

  // Third argument if existing = damping parameter
  if (argc >= 4)
    damping = atof(argv[3]);
}