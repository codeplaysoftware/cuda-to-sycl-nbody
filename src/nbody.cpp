// Copyright (C) 2016 - 2018 Sarah Le Luron
// Copyright (C) 2022 Codeplay Software Limited
 
#include <iostream>
#include <chrono>
#include <cstdlib>

#ifndef DISABLE_GL
#include <GL/glew.h>

#include "renderer_gl.hpp"
#include <GLFW/glfw3.h>
#endif

#include <glm/glm.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <numeric>
#include <algorithm>

#include "camera.hpp"
#include "gen.hpp"
#include "sim_param.hpp"
#include "simulator.cuh"

using namespace std;
using namespace simulation;

int main(int argc, char **argv) {

   SimParam params;
   params.parseArgs(argc, argv);

   DiskGalaxySimulator nbodySim(params);

#ifndef DISABLE_GL
   // Window initialization
   GLFWwindow *window;

   glfwSetErrorCallback([](const int error, const char *msg) {
      cout << "Error id : " << error << ", " << msg << endl;
      exit(-1);
   });

   if (!glfwInit()) {
      cout << "GLFW can't initialize" << endl;
      return -1;
   }

   GLFWmonitor *monitor = glfwGetPrimaryMonitor();

   const GLFWvidmode *mode = glfwGetVideoMode(monitor);

   glfwWindowHint(GLFW_RED_BITS, mode->redBits);
   glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
   glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
   glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
   RendererGL renderer;

   renderer.initWindow();

   int width = mode->width;
   int height = mode->height - 30;
   window = glfwCreateWindow(width, height, "N-Body Simulation", NULL, NULL);


   glfwMakeContextCurrent(window);

   renderer.init(window, width, height, nbodySim);
   renderer.initImgui(window);

   // Get initial postitions generated in simulator ctor
   renderer.updateParticles();

   Camera camera;

   float last_fps{0};
#endif

   std::vector<float> stepTimes;
   int step{0};

   // Main loop
   float stepTime = 0.0;

#ifndef DISABLE_GL
   while (!glfwWindowShouldClose(window) &&
          glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE &&
          step < params.numFrames) {
      double frame_start = glfwGetTime();
#else
   while ( step < params.numFrames) {
#endif
      nbodySim.stepSim();
#ifndef DISABLE_GL
      renderer.updateParticles();
      renderer.render(camera.getProj(width, height), camera.getView());
#endif
      if(!(step % 20)) stepTime = nbodySim.getLastStepTime();
#ifndef DISABLE_GL
      renderer.printKernelTime(stepTime);
#endif

      step++;
      int warmSteps{2};
      if (step > warmSteps) {
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
         std::cout << "At step " << step << " kernel time is "
                   << stepTimes.back() << " and mean is " << meanTime
                   << " and stddev is: " << stdDev << "\n";
      }
#ifndef DISABLE_GL
      // Window refresh
      glfwSwapBuffers(window);
      glfwPollEvents();

      // Thread sleep to match min frame time
      double frame_end = glfwGetTime();
      double elapsed = frame_end - frame_start;
      last_fps = 1.0 / elapsed;
#endif
   }
#ifndef DISABLE_GL
   renderer.destroy();
   glfwDestroyWindow(window);
   glfwTerminate();
#endif
   return 0;
}
