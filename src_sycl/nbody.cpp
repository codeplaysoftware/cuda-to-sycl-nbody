// Copyright (C) 2016 - 2018 Sarah Le Luron
// Copyright (C) 2022 Codeplay Software Limited
 
#include <iostream>

#include <GL/glew.h>

#include "renderer_gl.hpp"
#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdlib>
#include <glm/glm.hpp>
#include <iostream>
#include <thread>
#include <vector>

#include "camera.hpp"
#include "gen.hpp"
#include "sim_param.hpp"
#include "simulator.dp.hpp"


using namespace std;
using namespace simulation;

int main(int argc, char **argv) {

   SimParam params;
   params.parseArgs(argc, argv);

   DiskGalaxySimulator nbodySim(params);

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

   // Main loop
   while (!glfwWindowShouldClose(window) &&
          glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
      double frame_start = glfwGetTime();

      nbodySim.stepSim();
      renderer.updateParticles();
      renderer.render(camera.getProj(width, height), camera.getView());
      renderer.printKernelTime(nbodySim.getLastStepTime());

      // Window refresh
      glfwSwapBuffers(window);
      glfwPollEvents();

      // Thread sleep to match min frame time
      double frame_end = glfwGetTime();
      double elapsed = frame_end - frame_start;
      last_fps = 1.0 / elapsed;
   }
   renderer.destroy();
   glfwDestroyWindow(window);
   glfwTerminate();
   return 0;
}
