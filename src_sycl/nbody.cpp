// Copyright (C) 2016 - 2018 Sarah Le Luron
// Copyright (C) 2022 Codeplay Software Limited
 
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <numeric>
#include <algorithm>

#include "sim_param.hpp"
#include "simulator.dp.hpp"


using namespace std;
using namespace simulation;

int main(int argc, char **argv) {

   SimParam params;
   params.parseArgs(argc, argv);

   DiskGalaxySimulator nbodySim(params);
   float last_fps{0};

   std::vector<float> stepTimes;
   int stepCounter{0};

   // Main loop
   unsigned int counter = 0;
   float stepTime = 0.0;
   while(true){

      nbodySim.stepSim();
      if(!(counter % 20)) stepTime = nbodySim.getLastStepTime();

      stepCounter++;
      int warmSteps{2};
      if (stepCounter > warmSteps) {
         stepTimes.push_back(nbodySim.getLastStepTime());
         float cumStepTime =
             std::accumulate(stepTimes.begin(), stepTimes.end(), 0.0);
         float meanTime = cumStepTime / stepTimes.size();
         float accum{0.0};
         std::for_each(stepTimes.begin(), stepTimes.end(),
                       [&](const float time) {
                          accum += std::pow((time - meanTime), 2);
                       });
         float stdDev = std::pow(accum / stepTimes.size(), 0.5);
         std::cout << "At step " << stepCounter << " kernel time is "
                   << stepTimes.back() << " and mean is "
                   << cumStepTime / stepTimes.size()
                   << " and stddev is: " << stdDev << "\n";
      }
      counter++;
   }

   return 0;
}
