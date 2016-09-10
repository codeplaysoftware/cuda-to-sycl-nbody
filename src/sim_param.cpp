#include "sim_param.hpp"

#include <cstdlib>

sim_param sim_param_default()
{
  sim_param p;
  p.G = 2.0;
  p.frame_time = 1.0/60.0;
  p.dt = 0.005;
  p.num_particles = 50*256;
  p.max_iterations_per_frame = 4;
  p.damping = 0.999998;
  return p;
}

sim_param sim_param_parse_args(sim_param p, int argc, char **argv)
{
  if (argc >= 2)
    p.num_particles = 256*atoi(argv[1]);

  if (argc >= 3)
    p.max_iterations_per_frame = atoi(argv[2]);

  if (argc >= 4)
    p.damping = atof(argv[3]);
  return p;
}