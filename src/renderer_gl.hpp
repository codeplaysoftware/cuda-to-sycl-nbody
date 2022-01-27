#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "renderer.hpp"
#include "shader.hpp"

using namespace simulation;

class RendererGL : public Renderer {
  public:
   void initWindow();
   void init(GLFWwindow *window, int width, int height,
             simulation::Simulator &sim);
   void destroy();
   void updateParticles();
   void render(glm::mat4 proj_mat, glm::mat4 view_mat);
   RendererGL() : sim{} {}

  private:
   /// Provides the gl state with window dimensions for fbo size, etc
   void setWindowDimensions(int width, int height);

   /// Generates the star flare texture
   void createFlareTexture();

   /// Creates the VAO and VBO objects
   void createVaosVbos();

   /// Loads the shaders into the gl state
   void initShaders();

   // Initializes and supplies the framebuffers with valid data
   void initFbos();

   // Supplies the gl state with nbody simulation parameters
   void setUniforms();

   // Send data obtained from simulation to a buffer
   void setParticleData(const GLuint buffer, const ParticleData &data);

   Simulator *sim{nullptr};

   GLuint flareTex;         ///< Texture for the star flare
   GLuint vaoParticles;     ///< Vertex definition for points
   GLuint vboParticlesPos;  ///< Particle position buffer
   GLuint ssboVelocities;   ///< Particle velocity buffer
   GLuint vaoDeferred;      ///< Vertex definition for deferred
   GLuint vboDeferred;      ///< Vertex buffer of deferred fullscreen tri

   /** Shader programs **/
   ShaderProgram programHdr;      ///< HDR rendering step
   ShaderProgram programBlur;     ///< Bloom blurring step
   ShaderProgram programLum;      ///< Average luminance step
   ShaderProgram programTonemap;  ///< Tonemapping step

   GLuint fbos[4];     ///< FBOs (0 for hdr, 1 & 2 for blur ping pong, 3 for
                       ///< luminance)
   GLuint attachs[4];  ///< Respective FBO attachments.

   int texSize;        ///< Flare texture size in pixels
   int lumLod;         ///< Luminance texture level to sample from
   int blurDownscale;  ///< Downscale factor for the blurring step
   int width_;         ///< Viewport width
   int height_;        ///< Viewport height

   size_t numParticles;
   size_t computeIterations;
};