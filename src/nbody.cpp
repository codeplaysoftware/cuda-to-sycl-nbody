#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <vector>
#include <cstdlib>

#include <chrono>
#include <thread>

#include "sim_param.hpp"
#include "gen.hpp"
#include "renderer.hpp"
#include "camera.hpp"

double scroll = 0;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  scroll += yoffset;
}

using namespace std;

int main(int argc, char **argv)
{
  float sensibility = 0.002;
  float move_speed = 0.00005;

  sim_param params;
  params.parse_args(argc, argv);

  // Window initialization
  GLFWwindow *window;

  if (!glfwInit()) return -1;

  GLFWmonitor *monitor = glfwGetPrimaryMonitor();

  const GLFWvidmode *mode = glfwGetVideoMode(monitor);

  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  int width = mode->width;
  int height = mode->height-30;
  window = glfwCreateWindow(width, height, "N-Body Simulation", NULL, NULL);

  glfwMakeContextCurrent(window);
  glfwSetScrollCallback(window, scroll_callback);

  // OpenGL initialization

  GLenum error = glewInit();
  if (error != GLEW_OK)
  {
    cerr << "Can't load GL" << endl;
    return -1;
  }

  Renderer renderer;
  renderer.init(width, height, params);

  // Pos&vel init
  {
    vector<glm::vec4> particles_pos(params.num_particles);
    vector<glm::vec4> particles_vel(params.num_particles);
    for (size_t i=0;i<params.num_particles;++i)
    {
      glm::vec4 p = random_particle_pos();
      glm::vec4 v = random_particle_vel(p);

      particles_pos[i] = p;
      particles_vel[i] = v;
    }
    renderer.populateParticles(particles_pos, particles_vel);
  }

  Camera camera;

  // Mouse information
  double last_xpos, last_ypos;
  bool drag = false;
  glfwGetCursorPos(window, &last_xpos, &last_ypos);

  // Main loop
  while (!glfwWindowShouldClose(window) && 
    glfwGetKey(window, GLFW_KEY_ESCAPE)==GLFW_RELEASE)
  {
    double frame_start = glfwGetTime();

    // View movement
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT))
    {
      if (!drag)
      {
        glfwGetCursorPos(window, &last_xpos, &last_ypos);
        drag = true;
      }
      double xpos,ypos;
      double xdiff, ydiff;
      glfwGetCursorPos(window, &xpos, &ypos);
      xdiff = xpos - last_xpos;
      ydiff = ypos - last_ypos;

      last_xpos = xpos;
      last_ypos = ypos;

      camera.addVelocity(glm::vec3(xdiff, -ydiff, 0)*sensibility);
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT))
    {
      if (!drag)
      {
        glfwGetCursorPos(window, &last_xpos, &last_ypos);
        drag = true;
      }
      double xpos,ypos;
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

      camera.addLookAtVelocity(
        - (float)xdiff*move_speed*dist*xvec
        + (float)ydiff*move_speed*dist*yvec );
    }
    else drag = false;

    // Scrolling
    camera.addVelocity(glm::vec3(0,0,scroll*0.02));
    scroll = 0;

    camera.step();

    for (int i=0;i<params.max_iterations_per_frame;++i)
    {
      renderer.stepSim(params.num_particles);
    }

    renderer.render(params.num_particles, camera.getProj(width, height), camera.getView());

    // Window refresh
    glfwSwapBuffers(window);
    glfwPollEvents();

    // Thread sleep to match min frame time
    double frame_end = glfwGetTime();
    double elapsed = frame_end - frame_start;
    if (elapsed < params.frame_time) 
    {
      this_thread::sleep_for(chrono::nanoseconds(
        (long int)((params.frame_time-elapsed)*1000000000)));
    }
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}