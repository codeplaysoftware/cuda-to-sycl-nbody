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

   float G;                    ///< Gravitational parameter
   float dt;                   ///< Simulation delta t
   size_t numParticles;        ///< Number of particles simulated
   int maxIterationsPerFrame;  ///< Simulation iterations per frame rendered
   float damping;  ///< Damping parameter for simulating 'soupy' galaxy (1.0 =
                   ///< no damping)
   float
       distEps;  ///< Minimum distance to limit gravity of very close particles
};