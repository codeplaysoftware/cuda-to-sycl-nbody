#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cstdlib>

#include <chrono>
#include <thread>

#include "shader.hpp"
#include "sim_param.hpp"
#include "gen.hpp"

double scroll = 0;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  scroll += yoffset;
}

using namespace std;

struct gl_state
{
  GLuint flare_tex;
  GLuint vao_particles;
  GLuint vbo_particles_pos;
  GLuint vao_deferred;
  GLuint vbo_deferred;
  GLuint ssbo_velocities;

  GLuint program_interaction;
  GLuint program_integration;
  GLuint program_hdr;
  GLuint program_tonemap;
  GLuint program_highpass;
  GLuint program_blur;

  GLuint fbos[4];
  GLuint attachs[4];

  int tex_size;
  int blur_downscale;
  int width;
  int height;
};

void set_window_dimensions(gl_state &state, int width, int height);
void create_vaos_vbos(gl_state &state);

void set_window_dimensions(gl_state &state, int width, int height)
{
  state.width  = width;
  state.height = height;
}

void create_flare_texture(gl_state &state)
{
  int tex_size = 16;
  state.tex_size = tex_size;
  glCreateTextures(GL_TEXTURE_2D, 1, &state.flare_tex);
  glTextureStorage2D(state.flare_tex, 1, GL_R32F, tex_size, tex_size);
  glTextureParameteri(state.flare_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  {
    std::vector<float> pixels(tex_size*tex_size);
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
    glTextureSubImage2D(state.flare_tex, 0, 0, 0, 
      tex_size, tex_size, GL_RED, GL_FLOAT, pixels.data());
  }
}

void create_vaos_vbos(gl_state &state)
{
  // Particle VAO
  glCreateVertexArrays(1, &state.vao_particles);
  glCreateBuffers(1, &state.vbo_particles_pos);
  glCreateBuffers(1, &state.ssbo_velocities);
  glVertexArrayVertexBuffer(state.vao_particles, 0, state.vbo_particles_pos, 0, sizeof(glm::vec4));
  glVertexArrayVertexBuffer(state.vao_particles, 1, state.ssbo_velocities  , 0, sizeof(glm::vec4));

  glEnableVertexArrayAttrib( state.vao_particles, 0);
  glVertexArrayAttribFormat( state.vao_particles, 0, 4, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(state.vao_particles, 0, 0);

  glEnableVertexArrayAttrib( state.vao_particles, 1);
  glVertexArrayAttribFormat( state.vao_particles, 1, 4, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(state.vao_particles, 1, 1);

  // Deferred VAO
  glCreateVertexArrays(1, &state.vao_deferred);
  glCreateBuffers(1, &state.vbo_deferred);
  glVertexArrayVertexBuffer( state.vao_deferred, 0, state.vbo_deferred, 0, sizeof(glm::vec2));
  glEnableVertexArrayAttrib( state.vao_deferred, 0);
  glVertexArrayAttribFormat( state.vao_deferred, 0, 2, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(state.vao_deferred, 0, 0);

  // Deferred tri
  glm::vec2 tri[3] = {
    glm::vec2(-2,-1), 
    glm::vec2(+2,-1),
    glm::vec2( 0, 4)};
  glNamedBufferStorage(state.vbo_deferred, 3*sizeof(glm::vec2), tri, 0);
}

void populate_particles(gl_state &state, const vector<glm::vec4> pos, const vector<glm::vec4> vel)
{
  glNamedBufferStorage(state.vbo_particles_pos, pos.size()*sizeof(glm::vec4), pos.data(), 0);
  glNamedBufferStorage(state.ssbo_velocities  , vel.size()*sizeof(glm::vec4), vel.data(), 0);

  // SSBO binding
  glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, state.vbo_particles_pos, 
    0, sizeof(glm::vec4)*pos.size());
  glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, state.ssbo_velocities,
    0, sizeof(glm::vec4)*vel.size());
}

void init_shaders(gl_state &state)
{
  state.program_interaction = new_program();
  program_source(state.program_interaction, GL_COMPUTE_SHADER, "interaction.comp");
  program_link  (state.program_interaction);

  state.program_integration = new_program();
  program_source(state.program_integration, GL_COMPUTE_SHADER, "integration.comp");
  program_link  (state.program_integration);

  state.program_hdr = new_program();
  program_source(state.program_hdr, GL_VERTEX_SHADER  , "main.vert");
  program_source(state.program_hdr, GL_FRAGMENT_SHADER, "main.frag");
  program_source(state.program_hdr, GL_GEOMETRY_SHADER, "main.geom");
  program_link  (state.program_hdr);

  state.program_tonemap = new_program();
  program_source(state.program_tonemap, GL_VERTEX_SHADER,  "deferred.vert");
  program_source(state.program_tonemap, GL_FRAGMENT_SHADER,"tonemap.frag" );
  program_link  (state.program_tonemap);

  state.program_highpass = new_program();
  program_source(state.program_highpass, GL_VERTEX_SHADER,  "deferred.vert");
  program_source(state.program_highpass, GL_FRAGMENT_SHADER,"highpass.frag");
  program_link  (state.program_highpass);

  state.program_blur = new_program();
  program_source(state.program_blur, GL_VERTEX_SHADER,  "deferred.vert");
  program_source(state.program_blur, GL_FRAGMENT_SHADER,"blur.frag"    );
  program_link  (state.program_blur);
}

void init_fbos(gl_state &state)
{
  state.blur_downscale = 2;
  
  glCreateFramebuffers(4, state.fbos);
  glCreateTextures(GL_TEXTURE_2D, 4, state.attachs);

  for (int i=0;i<4;++i)
  {
    glTextureStorage2D(state.attachs[i], 1, GL_RGBA16F, 
      (i>1)?state.width/state.blur_downscale:state.width, 
      (i>1)?state.height/state.blur_downscale:state.height);
    glTextureParameteri(state.attachs[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(state.attachs[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(state.attachs[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(state.fbos[i], GL_COLOR_ATTACHMENT0, state.attachs[i], 0);
  }
}

void set_uniforms(gl_state &state, sim_param params)
{
  // const Uniforms
  glProgramUniform1f(state.program_interaction, 0, params.dt);
  glProgramUniform1f(state.program_interaction, 1, params.G);
  glProgramUniform1f(state.program_interaction, 2, params.damping);
  glProgramUniform1f(state.program_integration, 0, params.dt);
  glProgramUniform2f(state.program_hdr, 8, 
    state.tex_size/float(2*state.width), 
    state.tex_size/float(2*state.height));
  glProgramUniform3f(state.program_highpass, 0, 1.0,1.0,1.0);
  glProgramUniform2f(state.program_blur, 0, 
    (float)state.blur_downscale/state.width, 
    (float)state.blur_downscale/state.height);
}

void step_sim(gl_state &state, size_t num_particles)
{
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  glUseProgram(state.program_interaction);
  glDispatchCompute(num_particles/256, 1, 1);

  glUseProgram(state.program_integration);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  glDispatchCompute(num_particles/256, 1, 1);
}

void render(gl_state &state, size_t num_particles, glm::mat4 proj_mat, glm::mat4 view_mat)
{
  glBindVertexArray(state.vao_particles);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glBindFramebuffer(GL_FRAMEBUFFER, state.fbos[0]);
  glUseProgram(state.program_hdr);
  glClear(GL_COLOR_BUFFER_BIT);
  glProgramUniformMatrix4fv(state.program_hdr, 0, 1, GL_FALSE, glm::value_ptr(view_mat));
  glProgramUniformMatrix4fv(state.program_hdr, 4, 1, GL_FALSE, glm::value_ptr(proj_mat));
  glBindTextureUnit(0, state.flare_tex);
  glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
  glDrawArrays(GL_POINTS, 0, num_particles);

  glBindVertexArray(state.vao_deferred);
  glDisable(GL_BLEND);

  // Highpass
  glBindFramebuffer(GL_FRAMEBUFFER, state.fbos[1]);
  glUseProgram(state.program_highpass);
  
  glBindTextureUnit(0, state.attachs[0]);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  glViewport(0,0,state.width/state.blur_downscale, state.height/state.blur_downscale);
  glUseProgram(state.program_blur);

  // Blur pingpong
  int loop = 0;
  for (int i=0;i<2;++i)
  {
    if (i==0) glProgramUniform2f(state.program_blur, 1, 1, 0);
    else      glProgramUniform2f(state.program_blur, 1, 0, 1);
    for (int j=0;j<100;++j)
    {
      GLuint fbo = state.fbos[(loop%2)+2];
      GLuint attach = state.attachs[loop?((loop+1)%2+2):1];
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glBindTextureUnit(0, attach);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      loop++;
    }
  }

  glViewport(0,0,state.width,state.height);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glUseProgram(state.program_tonemap);
  glBindTextureUnit(0, state.attachs[0]);
  glBindTextureUnit(1, state.attachs[3]);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

int main(int argc, char **argv)
{
  double sensibility = 0.002;
  float move_speed = 0.5;

  sim_param params = sim_param_default();
  params = sim_param_parse_args(params, argc, argv);

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
  window = glfwCreateWindow(width, height, "N Body simulation", NULL, NULL);

  glfwMakeContextCurrent(window);
  glfwSetScrollCallback(window, scroll_callback);

  // OpenGL initialization

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    cerr << "Can't load GL" << endl;
    return -1;
  }

  gl_state state;

  set_window_dimensions(state, width, height);
  create_flare_texture(state);
  create_vaos_vbos(state);
  init_shaders(state);
  init_fbos(state);
  set_uniforms(state, params);

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
    populate_particles(state, particles_pos, particles_vel);
  }

  float phi = M_PI/4, theta = 0, dist = 50.0;
  float phi_vel = 0, theta_vel = 0, dist_vel = 0;

  double last_xpos, last_ypos;
  bool drag = false;
  glfwGetCursorPos(window, &last_xpos, &last_ypos);

  // Main loop
  while (!glfwWindowShouldClose(window) && 
    glfwGetKey(window, GLFW_KEY_ESCAPE)==GLFW_RELEASE)
  {
    double frame_start = glfwGetTime();

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT))
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

      theta_vel += xdiff*sensibility;
      phi_vel   += ydiff*sensibility;
    }
    else drag = false;

    theta -= theta_vel;
    phi += phi_vel;

    theta_vel *= 0.72;
    phi_vel *= 0.72;

    dist_vel += scroll*0.02;
    scroll = 0;
    dist *= (1.0-dist_vel);
    dist_vel *= 0.8;

    if (theta < 0) theta += 2*M_PI;
    if (theta >= 2*M_PI) theta -= 2*M_PI;
    phi = max(-(float)M_PI/2+0.001f, min(phi, (float)M_PI/2-0.001f));

    glm::vec3 view_pos = glm::vec3(cos(theta)*cos(phi), sin(theta)*cos(phi), sin(phi))*dist;

     glm::mat4 view_mat = glm::lookAt(
      view_pos,
      glm::vec3(0,0,0),
      glm::vec3(0,0,1));
    glm::mat4 proj_mat = glm::infinitePerspective(
    glm::radians(30.0f),mode->width/(float)mode->height, 1.f);

    for (int i=0;i<params.max_iterations_per_frame;++i)
    {
      step_sim(state, params.num_particles);
    }

    render(state, params.num_particles, proj_mat, view_mat);

    glfwSwapBuffers(window);
    glfwPollEvents();

    double frame_end = glfwGetTime();
    double elapsed = frame_end - frame_start;
    if (elapsed < params.frame_time) 
    {
      this_thread::sleep_for(chrono::nanoseconds((long int)((params.frame_time-elapsed)*1000000000)));
    }
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}