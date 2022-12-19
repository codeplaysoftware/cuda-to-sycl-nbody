// Copyright (C) 2016 - 2018 Sarah Le Luron
// Copyright (C) 2022 Codeplay Software Limited

#include "sim_param.hpp"

#include <cstdlib>
#include <cstdint>

SimParam::SimParam() {
   G = 2.0;
   dt = 0.005;
   numParticles = 50 * 256;
   numFrames = SIZE_MAX;
   simIterationsPerFrame = 4;
   damping = 0.999998;
   distEps = 1.0e-7;
}

void SimParam::parseArgs(int argc, char **argv) {
   // First argument if existing = number of particle batches (256 per batch)
   if (argc >= 2) numParticles = 256 * atoi(argv[1]);

   // Second argument if existing = number of iterations per frame
   if (argc >= 3) simIterationsPerFrame = atoi(argv[2]);

   // Third argument if existing = damping parameter
   if (argc >= 4) damping = atof(argv[3]);

   // Fourth argument if existing = dt (timestep size) parameter
   if (argc >= 5) dt = atof(argv[4]);

   // Fifth argument if existing = distEps (minimum inter-particle distance) parameter
   if (argc >= 6) distEps = atof(argv[5]);

   // Sixth argument if existing = G (gravity) parameter
   if (argc >= 7) G = atof(argv[6]);

   // Seventh argument if existing = number of frames to simulate
   if (argc >= 8) numFrames = atoi(argv[7]);

}