// Copyright (C) 2016 - 2018 Sarah Le Luron
// Copyright (C) 2022 Codeplay Software Limited

#include "sim_param.hpp"

#include <iostream>
#include <string>
#include <map>
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
  gwSize = 64;
  calcMethod = CalculationMethod::BRANCH;
}

// Set the calculation method from the given string
CalculationMethod getCalculationMethod(const std::string& method) {

  static const std::map<std::string, CalculationMethod> methodMap = {
    {"BRANCH", CalculationMethod::BRANCH},
    {"PREDICATED", CalculationMethod::PREDICATED}
  };

  auto it = methodMap.find(method);
  if (it != methodMap.end()) {
    return it->second;
  } else {
    throw std::invalid_argument("Valid calculation methods are BRANCH or PREDICATED");
  }
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

  // Eighth argument if existing = the work group size
  if (argc >= 9) gwSize = atoi(argv[8]);

  // Ninth argument if existing = the calculation method
  if (argc >= 10) calcMethod = getCalculationMethod(argv[9]);
}
