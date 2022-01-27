// Copyright (C) 2016 - 2018 Sarah Le Luron
// Copyright (C) 2022 Codeplay Software Limited

#include "renderer_gl.hpp"

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

#include "gen.hpp"

const int FBO_MARGIN = 50;

using namespace std;

void RendererGL::initWindow() {
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

// TODO - why isn't this a ctor?
void RendererGL::init(GLFWwindow *window, int width, int height,
                      simulation::Simulator &sim_) {
   // OpenGL initialization
   GLenum error = glewInit();
   if (error != GLEW_OK) {
      throw std::runtime_error("Can't load GL");
   }

   sim = &sim_;
   numParticles = sim->getNumParticles();
   setWindowDimensions(width, height);
   createFlareTexture();
   createVaosVbos();
   initShaders();
   initFbos();
   setUniforms();
}

void RendererGL::setWindowDimensions(int width, int height) {
   width_ = width;
   height_ = height;
}

void RendererGL::createFlareTexture() {
   texSize = 16;
   glCreateTextures(GL_TEXTURE_2D, 1, &flareTex);
   glTextureStorage2D(flareTex, 1, GL_R32F, texSize, texSize);
   glTextureParameteri(flareTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   {
      std::vector<float> pixels = genFlareTex(texSize);
      glTextureSubImage2D(flareTex, 0, 0, 0, texSize, texSize, GL_RED, GL_FLOAT,
                          pixels.data());
   }
}

void RendererGL::createVaosVbos() {
   // Particle VAO
   glCreateVertexArrays(1, &vaoParticles);
   glCreateBuffers(1, &vboParticlesPos);
   glCreateBuffers(1, &ssboVelocities);
   glVertexArrayVertexBuffer(vaoParticles, 0, vboParticlesPos, 0,
                             sizeof(glm::vec4));
   glVertexArrayVertexBuffer(vaoParticles, 1, ssboVelocities, 0,
                             sizeof(glm::vec4));

   // Position
   glEnableVertexArrayAttrib(vaoParticles, 0);
   glVertexArrayAttribFormat(vaoParticles, 0, 4, GL_FLOAT, GL_FALSE, 0);
   glVertexArrayAttribBinding(vaoParticles, 0, 0);

   // Velocity
   glEnableVertexArrayAttrib(vaoParticles, 1);
   glVertexArrayAttribFormat(vaoParticles, 1, 4, GL_FLOAT, GL_FALSE, 0);
   glVertexArrayAttribBinding(vaoParticles, 1, 1);

   // Deferred VAO
   glCreateVertexArrays(1, &vaoDeferred);
   glCreateBuffers(1, &vboDeferred);
   glVertexArrayVertexBuffer(vaoDeferred, 0, vboDeferred, 0, sizeof(glm::vec2));
   // Position
   glEnableVertexArrayAttrib(vaoDeferred, 0);
   glVertexArrayAttribFormat(vaoDeferred, 0, 2, GL_FLOAT, GL_FALSE, 0);
   glVertexArrayAttribBinding(vaoDeferred, 0, 0);

   // Deferred tri
   glm::vec2 tri[3] = {glm::vec2(-2, -1), glm::vec2(+2, -1), glm::vec2(0, 4)};
   glNamedBufferStorage(vboDeferred, 3 * sizeof(glm::vec2), tri, 0);

   // TODO - is GL_MAP_READ_BIT required here?
   // SSBO allocation (particle position & velocities)
   glNamedBufferStorage(vboParticlesPos, numParticles * sizeof(glm::vec4),
                        nullptr, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
   glNamedBufferStorage(ssboVelocities, numParticles * sizeof(glm::vec4),
                        nullptr, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

   // SSBO binding
   glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, vboParticlesPos, 0,
                     numParticles * sizeof(glm::vec4));
   glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, ssboVelocities, 0,
                     numParticles * sizeof(glm::vec4));
}

void RendererGL::updateParticles() {
   setParticleData(vboParticlesPos, sim->getParticlePos());
   setParticleData(ssboVelocities, sim->getParticleVel());
}

void RendererGL::setParticleData(const GLuint buffer,
                                 const ParticleData &data) {
   void *particle_ptr = glMapNamedBufferRange(
       buffer, 0, numParticles * sizeof(glm::vec4), GL_MAP_WRITE_BIT);

   assert(!glGetError());
   assert(particle_ptr);

   const ParticleData &particles = sim->getParticlePos();

   // Fill using placement new
   // TODO - can't set alpha channel - new vector member in ParticleData?
   // Optional arg?
   for (size_t i = 0; i < numParticles; i++) {
      glm::vec4 *my4 = new ((glm::vec4 *)particle_ptr + i)
          glm::vec4(data.x[i], data.y[i], data.z[i], 1.0f);
   }
   glUnmapNamedBuffer(buffer);
}

void RendererGL::initShaders() {
   // Need to cut these two shaders out
   // programInteraction.source(GL_COMPUTE_SHADER,
   // "shaders/gl/interaction.comp"); programInteraction.link();

   // programIntegration.source(GL_COMPUTE_SHADER,
   // "shaders/gl/integration.comp"); programIntegration.link();

   programHdr.source(GL_VERTEX_SHADER, "shaders/gl/main.vert");
   programHdr.source(GL_FRAGMENT_SHADER, "shaders/gl/main.frag");
   programHdr.source(GL_GEOMETRY_SHADER, "shaders/gl/main.geom");
   programHdr.link();

   programTonemap.source(GL_VERTEX_SHADER, "shaders/gl/deferred.vert");
   programTonemap.source(GL_FRAGMENT_SHADER, "shaders/gl/tonemap.frag");
   programTonemap.link();

   programBlur.source(GL_VERTEX_SHADER, "shaders/gl/deferred.vert");
   programBlur.source(GL_FRAGMENT_SHADER, "shaders/gl/blur.frag");
   programBlur.link();

   programLum.source(GL_VERTEX_SHADER, "shaders/gl/deferred.vert");
   programLum.source(GL_FRAGMENT_SHADER, "shaders/gl/luminance.frag");
   programLum.link();
}

void RendererGL::initFbos() {
   int blur_dsc = 2;
   blurDownscale = blur_dsc;

   glCreateFramebuffers(4, fbos);
   glCreateTextures(GL_TEXTURE_2D, 4, attachs);

   int base_width = width_ + 2 * FBO_MARGIN;
   int base_height = height_ + 2 * FBO_MARGIN;

   int widths[] = {base_width, base_width / blur_dsc, base_width / blur_dsc,
                   base_width / 2};

   int heights[] = {base_height, base_height / blur_dsc, base_height / blur_dsc,
                    base_height / 2};

   lumLod = (int)floor(log2(max(base_width, base_height) / 2));
   int mipmaps[] = {1, 1, 1, lumLod + 1};
   GLenum types[] = {GL_RGBA16F, GL_RGBA16F, GL_RGBA16F, GL_R16F};
   GLenum min_filters[] = {GL_LINEAR, GL_LINEAR, GL_LINEAR,
                           GL_LINEAR_MIPMAP_LINEAR};

   for (int i = 0; i < 4; ++i) {
      glTextureStorage2D(attachs[i], mipmaps[i], types[i], widths[i],
                         heights[i]);
      glTextureParameteri(attachs[i], GL_TEXTURE_MIN_FILTER, min_filters[i]);
      glTextureParameteri(attachs[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(attachs[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glNamedFramebufferTexture(fbos[i], GL_COLOR_ATTACHMENT0, attachs[i], 0);
   }
}

void RendererGL::setUniforms() {
   // // NDC sprite size
   glProgramUniform2f(programHdr.getId(), 8, texSize / float(2 * width_),
                      texSize / float(2 * height_));
   // Blur sample offset length
   glProgramUniform2f(programBlur.getId(), 0, (float)blurDownscale / width_,
                      (float)blurDownscale / height_);
}

void RendererGL::render(glm::mat4 proj_mat, glm::mat4 view_mat) {
   // Particle HDR rendering
   glViewport(0, 0, width_ + 2 * FBO_MARGIN, height_ + 2 * FBO_MARGIN);
   glBindVertexArray(vaoParticles);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);
   glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]);
   glUseProgram(programHdr.getId());
   glClear(GL_COLOR_BUFFER_BIT);
   glProgramUniformMatrix4fv(programHdr.getId(), 0, 1, GL_FALSE,
                             glm::value_ptr(view_mat));
   glProgramUniformMatrix4fv(programHdr.getId(), 4, 1, GL_FALSE,
                             glm::value_ptr(proj_mat));
   glBindTextureUnit(0, flareTex);
   glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
   glDrawArrays(GL_POINTS, 0, numParticles);

   glBindVertexArray(vaoDeferred);
   glDisable(GL_BLEND);

   glViewport(0, 0, (width_ + 2 * FBO_MARGIN) / blurDownscale,
              (height_ + 2 * FBO_MARGIN) / blurDownscale);
   glUseProgram(programBlur.getId());

   // Blur pingpong (N horizontal blurs then N vertical blurs)
   if (0) {
      int loop = 0;
      for (int i = 0; i < 2; ++i) {
         if (i == 0)
            glProgramUniform2f(programBlur.getId(), 1, 1, 0);
         else
            glProgramUniform2f(programBlur.getId(), 1, 0, 1);
         for (int j = 0; j < 100; ++j) {
            GLuint fbo = fbos[(loop % 2) + 1];
            GLuint attach = attachs[loop ? ((loop + 1) % 2 + 1) : 0];
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glBindTextureUnit(0, attach);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            loop++;
         }
      }
   }

   // Average luminance
   glViewport(0, 0, (width_ + 2 * FBO_MARGIN) / 2,
              (height_ + 2 * FBO_MARGIN) / 2);
   glBindFramebuffer(GL_FRAMEBUFFER, fbos[3]);
   glUseProgram(programLum.getId());
   glBindTextureUnit(0, attachs[0]);
   glDrawArrays(GL_TRIANGLES, 0, 3);
   glGenerateTextureMipmap(attachs[3]);

   // Tonemapping step (direct to screen)
   glViewport(0, 0, width_, height_);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glUseProgram(programTonemap.getId());
   glProgramUniform1i(programTonemap.getId(), 0, lumLod);
   glBindTextureUnit(0, attachs[0]);
   glBindTextureUnit(1, attachs[2]);
   glBindTextureUnit(2, attachs[3]);
   glDrawArrays(GL_TRIANGLES, 0, 3);
}

void RendererGL::destroy() {}