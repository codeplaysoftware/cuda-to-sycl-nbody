#include <iostream>

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>

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

  //window = glfwCreateWindow(mode->width, mode->height, "N Body simulation", monitor, NULL);
  window = glfwCreateWindow(800, 600, "N Body simulation", NULL, NULL);

  glfwMakeContextCurrent(window);

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
  const size_t NUM_PARTICLES = 8000;

  {
    std::vector<glm::vec4> particles_init(NUM_PARTICLES);
    for (size_t i=0;i<NUM_PARTICLES;++i)
    {
      particles_init[i] = random_particle();
    }
    glNamedBufferData(vbo, NUM_PARTICLES*2*sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);
    glNamedBufferSubData(vbo, 0, NUM_PARTICLES*sizeof(glm::vec4), particles_init.data());
  }

  // Shader program
  GLuint program_comp = glCreateProgram();
  program_source(program_comp, GL_COMPUTE_SHADER, "main.comp");
  program_link(program_comp);

  GLuint program_disp = glCreateProgram();
  program_source(program_disp, GL_VERTEX_SHADER  , "main.vert");
  program_source(program_disp, GL_FRAGMENT_SHADER, "main.frag");
  program_source(program_disp, GL_GEOMETRY_SHADER, "main.geom");
  program_link(program_disp);

  GLuint program_pipeline;
  glCreateProgramPipelines(1, &program_pipeline);
  glUseProgramStages(program_pipeline, GL_COMPUTE_SHADER_BIT, program_comp);
  glUseProgramStages(program_pipeline, 
    GL_VERTEX_SHADER_BIT|GL_GEOMETRY_SHADER_BIT|GL_FRAGMENT_SHADER_BIT,
    program_disp);

  glBindProgramPipeline(program_pipeline);

  bool double_buffer = false;

  glm::mat4 view_mat = glm::lookAt(glm::vec3(100,100,100),glm::vec3(0,0,0),glm::vec3(0,0,1));
  glm::mat4 proj_mat = glm::perspective(40.0f,4.f/3.f, 1.f,200.f);
  const float G = 0.001;

  glBindVertexArray(vao);

  // Main loop
  while (!glfwWindowShouldClose(window) && 
    glfwGetKey(window, GLFW_KEY_ESCAPE)==GLFW_RELEASE)
  {
    /*
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, vbo, 
      double_buffer?sizeof(glm::vec4)*NUM_PARTICLES:0,
      sizeof(glm::vec4)*NUM_PARTICLES);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, vbo, 
      double_buffer?0:sizeof(glm::vec4)*NUM_PARTICLES,
      sizeof(glm::vec4)*NUM_PARTICLES);
    glProgramUniform1f(program_comp, 8, G);
    glDispatchCompute(NUM_PARTICLES, 1, 1);
    */

    glClear(GL_COLOR_BUFFER_BIT);
    glProgramUniformMatrix4fv(program_disp, 0, 1, GL_FALSE, glm::value_ptr(view_mat));
    glProgramUniformMatrix4fv(program_disp, 4, 1, GL_FALSE, glm::value_ptr(proj_mat));
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  return 0;
}