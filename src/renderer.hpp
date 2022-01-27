#pragma once

#ifdef USE_OPENGL
#include <GL/glew.h>
#else
#ifdef USE_VULKAN
#include <vulkan/vulkan.h>
#endif
#endif
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <vector>

#include "simulator.cuh"

class Renderer {
  public:
   virtual void initWindow() = 0;

   /**
    * Initializes the gl state
    * @param width viewport width
    * @param height viewport height
    * @param params simulation parameters
    */
   virtual void init(GLFWwindow *window, int width, int height,
                     simulation::Simulator &sim) = 0;

   virtual void destroy() = 0;

   /**
    * Supplies the gl state with updated particle position and velocity
    * @param pos particle positions
    * @param vel particle velocities
    */
   virtual void updateParticles() = 0;

   /**
    * Renders the particles at the current step
    * @param proj_mat projection matrix @see camera_get_proj
    * @param view_mat view matrix @see camera_get_view
    */
   virtual void render(glm::mat4 projMat, glm::mat4 viewMat) = 0;
};