// Copyright (C) 2016 - 2018 Sarah Le Luron
// Copyright (C) 2022 Codeplay Software Limited

#include "sim_param.hpp"

#include <cstdlib>

SimParam::SimParam() {
   G = 2.0;
   dt = 0.005;
   numParticles = 50 * 256;
   maxIterationsPerFrame = 4;
   damping = 0.999998;
   distEps = 1.0e-7;
}

void SimParam::parseArgs(int argc, char **argv) {
   // First argument if existing = number of particle batches (256 per batch)
   if (argc >= 2) numParticles = 256 * atoi(argv[1]);

   // Second argument if existing = number of iterations per frame
   if (argc >= 3) maxIterationsPerFrame = atoi(argv[2]);

   // Third argument if existing = damping parameter
   if (argc >= 4) damping = atof(argv[3]);
}