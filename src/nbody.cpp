// Copyright (C) 2016 - 2018 Sarah Le Luron
// Copyright (C) 2022 Codeplay Software Limited
 
#include <iostream>

#ifdef USE_OPENGL
#include <GL/glew.h>

#include "renderer_gl.hpp"
#else
#ifdef USE_VULKAN
#include <vulkan/vulkan.h>

#include "renderer_vk.hpp"
#endif
#endif
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
#include "simulator.cuh"

double scroll = 0;

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
   scroll += yoffset;
}

using namespace std;
using namespace simulation;

int main(int argc, char **argv) {
   float sensibility = 0.002;
   float move_speed = 0.00005;

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

#ifdef USE_OPENGL
   RendererGL renderer;
#else
#ifdef USE_VULKAN
   RendererVk renderer;
#endif
#endif

   renderer.initWindow();

   int width = mode->width;
   int height = mode->height - 30;
   window = glfwCreateWindow(width, height, "N-Body Simulation", NULL, NULL);

   glfwMakeContextCurrent(window);
   glfwSetScrollCallback(window, scroll_callback);

   renderer.init(window, width, height, nbodySim);
   renderer.initImgui(window);

   // Get initial postitions generated in simulator ctor
   renderer.updateParticles();

   Camera camera;

   // Mouse information
   double last_xpos = 0.0, last_ypos = 0.0;
   bool drag = false;
   glfwGetCursorPos(window, &last_xpos, &last_ypos);

   double last_fps{0};

   // Main loop
   while (!glfwWindowShouldClose(window) &&
          glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
      double frame_start = glfwGetTime();

      // View movement
      if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
         if (!drag) {
            glfwGetCursorPos(window, &last_xpos, &last_ypos);
            drag = true;
         }
         double xpos, ypos;
         double xdiff, ydiff;
         glfwGetCursorPos(window, &xpos, &ypos);
         xdiff = xpos - last_xpos;
         ydiff = ypos - last_ypos;

         last_xpos = xpos;
         last_ypos = ypos;

         camera.addVelocity(glm::vec3(xdiff, -ydiff, 0) * sensibility);
      } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)) {
         if (!drag) {
            glfwGetCursorPos(window, &last_xpos, &last_ypos);
            drag = true;
         }
         double xpos, ypos;
         double xdiff, ydiff;
         glfwGetCursorPos(window, &xpos, &ypos);
         xdiff = xpos - last_xpos;
         ydiff = ypos - last_ypos;

         last_xpos = xpos;
         last_ypos = ypos;

         glm::vec3 xvec = camera.getRight();
         xvec.z = 0.0;
         xvec = glm::normalize(xvec);

         glm::vec3 yvec = camera.getUp();
         yvec.z = 0.0;
         yvec = glm::normalize(yvec);

         float dist = camera.getPosition().z;

         camera.addLookAtVelocity(-(float)xdiff * move_speed * dist * xvec +
                                  (float)ydiff * move_speed * dist * yvec);
      } else
         drag = false;

      // Scrolling
      camera.addVelocity(glm::vec3(0, 0, scroll * 0.02));
      scroll = 0;

      camera.step();

      std::cout << "Updating simulation" << std::endl;
      nbodySim.stepSim();
      std::cout << "Updating particle pos" << std::endl;
      renderer.updateParticles();
      std::cout << "Rendering" << std::endl;
      renderer.render(camera.getProj(width, height), camera.getView());
      renderer.printFPS(last_fps);

      // Window refresh
      glfwSwapBuffers(window);
      glfwPollEvents();

      // Thread sleep to match min frame time
      double frame_end = glfwGetTime();
      double elapsed = frame_end - frame_start;
      last_fps = 1.0 / elapsed;

      std::cout << "FPS: " << last_fps << "\nElapsed: " << elapsed << "\n";
   }
   renderer.destroy();
   glfwDestroyWindow(window);
   glfwTerminate();
   return 0;
}
