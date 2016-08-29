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
#include <random>

const float G = 2.0;
const float frame_time = 1.0/60.0;
const float dt = 0.005;

using namespace std;

mt19937 rng;
uniform_real_distribution<> dis(0, 1);

glm::vec4 random_particle()
{
  /*
  glm::vec3 euler; 
  euler.x = dis(rng)*2*M_PI;
  euler.y = acos(2*dis(rng)-1)-M_PI/2;
  euler.z = 40.0;

  glm::vec4 particle;
  particle.x = cos(euler.x)*cos(euler.y)*euler.z;
  particle.y = sin(euler.x)*cos(euler.y)*euler.z;
  particle.z = sin(euler.y)*euler.z;
  */

  glm::vec4 particle;
  float t = dis(rng)*2*M_PI;
  float s = dis(rng)*100;
  particle.x = cos(t)*s;
  particle.y = sin(t)*s;
  particle.z = dis(rng)*4;

  particle.w = 1.f;
  return particle;
}

glm::vec4 random_vel(glm::vec4 pos)
{
  glm::vec3 vel = glm::cross(glm::vec3(pos),glm::vec3(0,0,1));
  float orbital_vel = sqrt(2.0*glm::length(vel));
  vel = glm::normalize(vel)*orbital_vel;
  return glm::vec4(vel,0.0);
}

/*
glm::vec3 temp_to_color(float temp)
{
  glm::vec3 color;
  temp /= 100.0;

  if (temp <= 66)
  {
    color.r = 1.0;
    float g = 99.4708025861 * log(temp) - 161.1195681661;
    color.g = max(0.f, min(1.f, g/255.f));
  }
  else
  {
    float r = 329.698727446 * pow(temp-60, -0.1332047592);
    color.r = max(0.f, min(1.f, r/255.f));
    float g  = 288.1221695283*pow(temp,-0.0755148492);
    color.g = max(0.f, min(1.f, g/255.f));
  }

  if (temp >= 66)      color.b = 1.0;
  else if (temp <= 19) color.b = 0.0;
  else
  {
    float b = 138.5177312231*log(temp-10) - 305.0447927307;
    color.b = max(0.f, min(1.f, b/255.f));
  }
  return color;
}

float temp_to_luminosity(float temp)
{
  return 1.807897117E-11*pow(temp/100.0,6.122745955);
}

glm::vec4 random_color()
{
  //float temp = dis(rng)*8000+14000;
  //return glm::vec4(temp_to_color(temp)*(temp_to_luminosity(temp)*0.01f), 1.0);
  float t = dis(rng);
  glm::vec3 c = glm::vec3(0.6*t,0.4,1.0);
  return glm::vec4(c*10.f,1.f);
}
*/

void program_source(GLuint program, GLenum shader_type, const string &filename)
{
  string code;

  try
  {
    stringstream sstream;
    {
      ifstream stream;
      stream.exceptions(ifstream::failbit | ifstream::badbit);
      stream.open(filename);
      sstream << stream.rdbuf();
    }
    code = sstream.str();
  }
  catch (ifstream::failure e)
  {
    cerr << "Can't open " << filename << " : " << e.what() << endl;
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
    cerr << "Can't compile " <<  filename << " " << info_log << endl;
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
    cerr << "Can't link :" << info_log << endl;
  }
}

int main(int argc, char **argv)
{

  size_t num_particles = 50*256;
  int MAX_ITERATIONS_PER_FRAME = 4;
  float damping = 0.9998;

  float view_theta = 5*M_PI/4;
  float view_phi = -M_PI/4;

  double sensibility = 0.002;
  float move_speed = 0.5;

  if (argc >= 2)
    num_particles = 256*atoi(argv[1]);

  if (argc >= 3)
    MAX_ITERATIONS_PER_FRAME = atoi(argv[2]);

  if (argc >= 4)
    damping = atof(argv[3]);

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

  int width = 1280;
  int height = 720;
  window = glfwCreateWindow(width, height, "N Body simulation", NULL, NULL);

  glfwMakeContextCurrent(window);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // OpenGL initialization

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    cerr << "Can't load GL" << endl;
    return -1;
  }

  // Flare texture init
  GLuint flare_tex;
  const int tex_size = 16;
  glCreateTextures(GL_TEXTURE_2D, 1, &flare_tex);
  glTextureStorage2D(flare_tex, 1, GL_R32F, tex_size, tex_size);
  glTextureParameteri(flare_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  {
    std::array<float, tex_size*tex_size> pixels;
    float sigma2 = tex_size/2.0;
    float A = 1.0;
    for (int i=0;i<tex_size;++i)
    {
      float i1 = i-tex_size/2;
      for (int j=0;j<tex_size;++j)
      {
        float j1 = j-tex_size/2;
        pixels[i*tex_size+j] = pow(A*exp(-((i1*i1)/(2*sigma2) + (j1*j1)/(2*sigma2))),2.2);
      }
    }
    glTextureSubImage2D(flare_tex, 0, 0, 0, tex_size, tex_size, GL_RED, GL_FLOAT, &pixels);
  }

  // Enables
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  // VAO init
  // Particle VAO
  GLuint vao_particles, vbo_pos;
  // Velocity SSBO
  GLuint velocities;
  glCreateVertexArrays(1, &vao_particles);
  glCreateBuffers(1, &vbo_pos);
  glCreateBuffers(1, &velocities);
  glVertexArrayVertexBuffer(vao_particles, 0, vbo_pos, 0, sizeof(glm::vec4));
  glVertexArrayVertexBuffer(vao_particles, 1, velocities, 0, sizeof(glm::vec4));

  glEnableVertexArrayAttrib( vao_particles, 0);
  glVertexArrayAttribFormat( vao_particles, 0, 4, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(vao_particles, 0, 0);

  glEnableVertexArrayAttrib( vao_particles, 1);
  glVertexArrayAttribFormat( vao_particles, 1, 4, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(vao_particles, 1, 1);


  // Deferred VAO
  GLuint vao_deferred, vbo_deferred;
  glCreateVertexArrays(1, &vao_deferred);
  glCreateBuffers(1, &vbo_deferred);
  glVertexArrayVertexBuffer(vao_deferred, 0, vbo_deferred, 0, sizeof(glm::vec2));
  glEnableVertexArrayAttrib( vao_deferred, 0);
  glVertexArrayAttribFormat( vao_deferred, 0, 2, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(vao_deferred, 0, 0);

  // Deferred tri
  {
    glm::vec2 tri[3] = {
      glm::vec2(-2,-1), 
      glm::vec2(+2,-1),
      glm::vec2( 0, 4)};
    glNamedBufferStorage(vbo_deferred, 3*sizeof(glm::vec2), tri, 0);
  }

  // Pos&vel SSBO init
  {
    vector<glm::vec4> particles_pos(num_particles);
    for (size_t i=0;i<num_particles;++i)
    {
      particles_pos[i] = random_particle();
    }
    glNamedBufferStorage(vbo_pos, num_particles*sizeof(glm::vec4), particles_pos.data(), 0);

    vector<glm::vec4> vel_init(num_particles);
    for (size_t i=0;i<num_particles;++i)
    {
      vel_init[i] = random_vel(particles_pos[i]);
    }
    glNamedBufferStorage(velocities, num_particles*sizeof(glm::vec4), vel_init.data(), 0);
  }

  // Shader program
  GLuint program_interaction = glCreateProgram();
  program_source(program_interaction, GL_COMPUTE_SHADER, "interaction.comp");
  program_link(program_interaction);

  GLuint program_integration = glCreateProgram();
  program_source(program_integration, GL_COMPUTE_SHADER, "integration.comp");
  program_link(program_integration);

  GLuint program_hdr = glCreateProgram();
  program_source(program_hdr, GL_VERTEX_SHADER  , "main.vert");
  program_source(program_hdr, GL_FRAGMENT_SHADER, "main.frag");
  program_source(program_hdr, GL_GEOMETRY_SHADER, "main.geom");
  program_link(program_hdr);

  GLuint program_tonemap = glCreateProgram();
  program_source(program_tonemap, GL_VERTEX_SHADER,  "deferred.vert");
  program_source(program_tonemap, GL_FRAGMENT_SHADER,"tonemap.frag" );
  program_link(program_tonemap);

  GLuint program_highpass = glCreateProgram();
  program_source(program_highpass, GL_VERTEX_SHADER,  "deferred.vert");
  program_source(program_highpass, GL_FRAGMENT_SHADER,"highpass.frag");
  program_link(program_highpass);

  GLuint program_blur = glCreateProgram();
  program_source(program_blur, GL_VERTEX_SHADER,  "deferred.vert");
  program_source(program_blur, GL_FRAGMENT_SHADER,"blur.frag"    );
  program_link(program_blur);

  // FBOs
  int blur_downscale = 2;
  GLuint fbos[4];
  GLuint attachs[4];
  glCreateFramebuffers(4, fbos);
  glCreateTextures(GL_TEXTURE_2D, 4, attachs);

  for (int i=0;i<4;++i)
  {
    glTextureStorage2D(attachs[i], 1, GL_RGBA16F, 
      (i>1)?width/blur_downscale:width, 
      (i>1)?height/blur_downscale:height);
    glTextureParameteri(attachs[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(attachs[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(attachs[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(fbos[i], GL_COLOR_ATTACHMENT0, attachs[i], 0);
  }

  // const Uniforms
  glProgramUniform1f(program_interaction, 0, dt);
  glProgramUniform1f(program_interaction, 1, G);
  glProgramUniform1f(program_interaction, 2, damping);
  glProgramUniform1f(program_integration, 0, dt);
  glProgramUniform2f(program_hdr, 8, tex_size/float(2*width), tex_size/float(2*height));
  glProgramUniform3f(program_highpass, 0, 1.0,1.0,1.0);
  glProgramUniform2f(program_blur, 0, 
    (float)blur_downscale/width, 
    (float)blur_downscale/height);

  // SSBO binding
  glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, vbo_pos, 
    0, sizeof(glm::vec4)*num_particles);
  glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, velocities,
    0, sizeof(glm::vec4)*num_particles);

  int iterations_per_frame = MAX_ITERATIONS_PER_FRAME;

  glm::vec3 view_pos = glm::vec3(100,100,100);

  double xmid = mode->width  / 2;
  double ymid = mode->height / 2;

  double last_xpos, last_ypos;
  glfwGetCursorPos(window, &last_xpos, &last_ypos);

  // Main loop
  while (!glfwWindowShouldClose(window) && 
    glfwGetKey(window, GLFW_KEY_ESCAPE)==GLFW_RELEASE)
  {
    double frame_start = glfwGetTime();

    double xpos,ypos;
    double xdiff, ydiff;
    glfwGetCursorPos(window, &xpos, &ypos);
    xdiff = xpos - last_xpos;
    ydiff = ypos - last_ypos;

    last_xpos = xpos;
    last_ypos = ypos;

    view_theta -= xdiff*sensibility;
    view_phi -= ydiff*sensibility;

    if (view_theta < 0) view_theta += 2*M_PI;
    if (view_theta >= 2*M_PI) view_theta -= 2*M_PI;
    view_phi = max(-(float)M_PI/2+0.001f, min(view_phi, (float)M_PI/2-0.001f));

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
      glDispatchCompute(num_particles/256, 1, 1);

      glUseProgram(program_integration);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      glDispatchCompute(num_particles/256, 1, 1);
    }

    glBindVertexArray(vao_particles);

    glEnable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]);
    glUseProgram(program_hdr);
    glClear(GL_COLOR_BUFFER_BIT);
    glProgramUniformMatrix4fv(program_hdr, 0, 1, GL_FALSE, glm::value_ptr(view_mat));
    glProgramUniformMatrix4fv(program_hdr, 4, 1, GL_FALSE, glm::value_ptr(proj_mat));
    glBindTextureUnit(0, flare_tex);
    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    glDrawArrays(GL_POINTS, 0, num_particles);

    glBindVertexArray(vao_deferred);
    glDisable(GL_BLEND);

    // Highpass
    glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]);
    glUseProgram(program_highpass);
    
    glBindTextureUnit(0, attachs[0]);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glViewport(0,0,width/blur_downscale, height/blur_downscale);
    glUseProgram(program_blur);

    // Blur pingpong
    int loop = 0;
    for (int i=0;i<2;++i)
    {
      if (i==0) glProgramUniform2f(program_blur, 1, 1, 0);
      else      glProgramUniform2f(program_blur, 1, 0, 1);
      for (int j=0;j<100;++j)
      {
        GLuint fbo = fbos[(loop%2)+2];
        GLuint attach = attachs[loop?((loop+1)%2+2):1];
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glBindTextureUnit(0, attach);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        loop++;
      }
    }

    glViewport(0,0,width,height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(program_tonemap);
    glBindTextureUnit(0, attachs[0]);
    glBindTextureUnit(1, attachs[3]);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents();

    double frame_end = glfwGetTime();
    double elapsed = frame_end - frame_start;
    if (elapsed < frame_time) 
    {
      this_thread::sleep_for(chrono::nanoseconds((long int)((frame_time-elapsed)*1000000000)));
    }
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}