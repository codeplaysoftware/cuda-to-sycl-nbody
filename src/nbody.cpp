#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>

#include <chrono>
#include <thread>

const float G = 10.0;
const float frame_time = 1.0/60.0;
const size_t NUM_PARTICLES = 8*256;
const int MAX_ITERATIONS_PER_FRAME = 16;

glm::vec4 random_particle()
{
  glm::vec4 particle;
  for (int i=0;i<3;++i)
  {
    particle[i] = 200.0*std::rand()/(float)RAND_MAX - 100.0;
  }
  particle.w = 10.0*std::rand()/(float)RAND_MAX;
  return particle;
}

void program_source(GLuint program, GLenum shader_type, const std::string &filename)
{
  std::string code;

  try
  {
    std::stringstream sstream;
    {
      std::ifstream stream;
      stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      stream.open(filename);
      sstream << stream.rdbuf();
    }
    code = sstream.str();
  }
  catch (std::ifstream::failure e)
  {
    std::cerr << "Can't open " << filename << " : " << e.what() << std::endl;
  }

  GLint success;
  GLchar info_log[2048];

  const char *s = code.c_str();

  GLuint shad_id = glCreateShader(shader_type);
  glShaderSource(shad_id, 1, &s, NULL);
  glCompileShader(shad_id);
  glGetShaderiv(shad_id, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(shad_id, sizeof(info_log), NULL, info_log);
    std::cerr << "Can't compile " <<  filename << " " << info_log << std::endl;
    exit(-1);
  }
  glAttachShader(program, shad_id);
}

void program_link(GLuint program)
{
  GLint success;
  GLchar info_log[2048];

  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success)
  {
    glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
    std::cerr << "Can't link :" << info_log << std::endl;
  }
}

int main()
{
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

  window = glfwCreateWindow(mode->width, mode->height, "N Body simulation", monitor, NULL);

  glfwMakeContextCurrent(window);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

  // OpenGL initialization

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cerr << "Can't load GL" << std::endl;
    return -1;
  }

  // VAO init
  GLuint vao, vbo;
  glCreateVertexArrays(1, &vao);
  glCreateBuffers(1, &vbo);
  glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(glm::vec4));

  glEnableVertexArrayAttrib(vao, 0);
  glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(vao, 0, 0);

  // SSBO init
  {
    std::vector<glm::vec4> particles_init(NUM_PARTICLES);
    for (size_t i=0;i<NUM_PARTICLES;++i)
    {
      particles_init[i] = random_particle();
    }
    glNamedBufferData(vbo, NUM_PARTICLES*sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);
    glNamedBufferSubData(vbo, 0, NUM_PARTICLES*sizeof(glm::vec4), particles_init.data());
  }

  // Velocity SSBO
  GLuint velocities;
  glCreateBuffers(1, &velocities);

  {
    std::vector<glm::vec4> vel_init(NUM_PARTICLES);
    for (size_t i=0;i<NUM_PARTICLES;++i)
    {
      vel_init[i] = glm::vec4(0.0);
    }
    glNamedBufferData(velocities, NUM_PARTICLES*sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);
    glNamedBufferSubData(velocities, 0, NUM_PARTICLES*sizeof(glm::vec4), vel_init.data());
  }

  // Shader program
  GLuint program_interaction = glCreateProgram();
  program_source(program_interaction, GL_COMPUTE_SHADER, "interaction.comp");
  program_link(program_interaction);

  GLuint program_integration = glCreateProgram();
  program_source(program_integration, GL_COMPUTE_SHADER, "integration.comp");
  program_link(program_integration);

  GLuint program_disp = glCreateProgram();
  program_source(program_disp, GL_VERTEX_SHADER  , "main.vert");
  program_source(program_disp, GL_FRAGMENT_SHADER, "main.frag");
  program_source(program_disp, GL_GEOMETRY_SHADER, "main.geom");
  program_link(program_disp);

  // Uniforms
  glProgramUniform1f(program_interaction, 0, frame_time);
  glProgramUniform1f(program_interaction, 1, G);

  glProgramUniform1f(program_integration, 0, frame_time);

  // SSBO binding
  glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, vbo, 
    0, sizeof(glm::vec4)*NUM_PARTICLES);
  glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, velocities,
    0, sizeof(glm::vec4)*NUM_PARTICLES);

  glBindVertexArray(vao);

  int iterations_per_frame = MAX_ITERATIONS_PER_FRAME;

  float view_theta = 0.0;
  float view_phi = 0.0;

  glm::vec3 view_pos = glm::vec3(100,100,100);

  double xmid = mode->width  / 2;
  double ymid = mode->height / 2;

  double sensibility = 0.002;
  float move_speed = 0.5;

  // Main loop
  while (!glfwWindowShouldClose(window) && 
    glfwGetKey(window, GLFW_KEY_ESCAPE)==GLFW_RELEASE)
  {
    double frame_start = glfwGetTime();

    double xpos,ypos;
    double xdiff, ydiff;
    glfwGetCursorPos(window, &xpos, &ypos);
    xdiff = xpos - xmid;
    ydiff = ypos - ymid;
    glfwSetCursorPos(window, xmid, ymid);

    view_theta -= xdiff*sensibility;
    view_phi -= ydiff*sensibility;

    view_phi = std::max(-(float)M_PI/2+0.0001f, std::min(view_phi, (float)M_PI/2-0.0001f));

    glm::vec3 dir = glm::vec3(cos(view_theta)*cos(view_phi), sin(view_theta)*cos(view_phi), sin(view_phi));
    glm::vec3 right = glm::normalize(glm::cross(dir,glm::vec3(0,0,1)));
    glm::vec3 up = glm::normalize(glm::cross(right, dir));

    if (glfwGetKey(window, GLFW_KEY_W))
      view_pos += move_speed*dir;
    else if (glfwGetKey(window, GLFW_KEY_S))
      view_pos -= move_speed*dir;
    if (glfwGetKey(window, GLFW_KEY_D))
      view_pos += move_speed*right;
    else if (glfwGetKey(window, GLFW_KEY_A))
      view_pos -= move_speed*right;

     glm::mat4 view_mat = glm::lookAt(
      view_pos,
      view_pos+dir,
      glm::vec3(0,0,1));
    glm::mat4 proj_mat = glm::infinitePerspective(
    glm::radians(30.0f),mode->width/(float)mode->height, 1.f);

    for (int i=0;i<iterations_per_frame;++i)
    {
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      glUseProgram(program_interaction);
      glDispatchCompute(NUM_PARTICLES/256, 1, 1);

      glUseProgram(program_integration);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      glDispatchCompute(NUM_PARTICLES/256, 1, 1);
    }

    glUseProgram(program_disp);
    glClear(GL_COLOR_BUFFER_BIT);
    glProgramUniformMatrix4fv(program_disp, 0, 1, GL_FALSE, glm::value_ptr(view_mat));
    glProgramUniformMatrix4fv(program_disp, 4, 1, GL_FALSE, glm::value_ptr(proj_mat));
    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

    glfwSwapBuffers(window);
    glfwPollEvents();

    double frame_end = glfwGetTime();
    double elapsed = frame_end - frame_start;
    if (elapsed < frame_time) 
    {
      std::this_thread::sleep_for(std::chrono::nanoseconds((long int)((frame_time-elapsed)*1000000000)));
    }
  }

  glfwDestroyWindow(window);
  return 0;
}