// Copyright (C) 2016 - 2018 Sarah Le Luron
// Copyright (C) 2022 Codeplay Software Limited

#pragma once

#include <cstdlib>

/**
 * Simulation parameters
 */
class SimParam {
  public:
    /**
     * Creates default simulation parameters
     */
    SimParam();

    /**
     * Provides user-defined simulation parameters
     * @param argc number of arguments
     * @param argv arguments
     */
    void parseArgs(int argc, char **argv);

    float G;                     ///< Gravitational parameter
    float dt;                    ///< Simulation delta t
    size_t numParticles;         ///< Number of particles simulated
    size_t numFrames;            ///< Number of frames simulated
    int simIterationsPerFrame;   ///< Simulation iterations per frame rendered
    float damping;  ///< Damping parameter for simulating 'soupy' galaxy (1.0 =
                    ///< no damping)
    float distEps;  ///< Minimum distance to limit gravity of very close particles
    int gwSize;                  ///< Work group size
    bool useBranch;              /// Use or not branch instruction in kernel
};
