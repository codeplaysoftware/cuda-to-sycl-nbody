#include "renderer.hpp"

#include <GL/glew.h>
#include "shader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

const int FBO_MARGIN = 50;

using namespace std;

/**
 * Contains the gl state necessary to run the nbody simulation
 */
struct RendererImpl
{
  GLuint flare_tex;           ///< Texture for the star flare
  GLuint vao_particles;       ///< Vertex definition for points
  GLuint vbo_particles_pos;   ///< Particle position buffer
  GLuint ssbo_velocities;     ///< Particle velocity buffer
  GLuint vao_deferred;        ///< Vertex definition for deferred
  GLuint vbo_deferred;        ///< Vertex buffer of deferred fullscreen tri

  /** Shader programs **/
  ShaderProgram program_interaction; ///< Gravity interaction step
  ShaderProgram program_integration; ///< Position integration step
  ShaderProgram program_hdr;         ///< HDR rendering step
  ShaderProgram program_blur;        ///< Bloom blurring step
  ShaderProgram program_lum;         ///< Average luminance step
  ShaderProgram program_tonemap;     ///< Tonemapping step

  GLuint fbos[4];             ///< FBOs (0 for hdr, 1 & 2 for blur ping pong, 3 for luminance)
  GLuint attachs[4];          ///< Respective FBO attachments.

  int tex_size;               ///< Flare texture size in pixels
  int lum_lod;                ///< Luminance texture level to sample from
  int blur_downscale;         ///< Downscale factor for the blurring step
  int width;                  ///< Viewport width
  int height;                 ///< Viewport height
};

Renderer::Renderer()
{
  impl = new RendererImpl();
}

Renderer::~Renderer()
{
  delete impl;
}

/// Provides the gl state with window dimensions for fbo size, etc
void setWindowDimensions(RendererImpl *impl, int width, int height);

/// Generates the star flare texture
void CreateFlareTexture(RendererImpl *impl);

/// Creates the VAO and VBO objects
void createVaosVbos(RendererImpl *impl);

/// Loads the shaders into the gl state
void initShaders(RendererImpl *impl);

// Initializes and supplies the framebuffers with valid data
void initFbos(RendererImpl *impl);

// Supplies the gl state with nbody simulation parameters
void setUniforms(RendererImpl *impl, sim_param params);

void Renderer::init(int width, int height, sim_param params)
{
  setWindowDimensions(impl, width, height);
  CreateFlareTexture(impl);
  createVaosVbos(impl);
  initShaders(impl);
  initFbos(impl);
  setUniforms(impl, params);
}

void setWindowDimensions(RendererImpl *impl, int width, int height)
{
  impl->width  = width;
  impl->height = height;
}

void CreateFlareTexture(RendererImpl *impl)
{
  int tex_size = 16;
  impl->tex_size = tex_size;
  glCreateTextures(GL_TEXTURE_2D, 1, &impl->flare_tex);
  glTextureStorage2D(impl->flare_tex, 1, GL_R32F, tex_size, tex_size);
  glTextureParameteri(impl->flare_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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
        // gamma corrected gauss
        pixels[i*tex_size+j] = pow(A*exp(-((i1*i1)/(2*sigma2) + (j1*j1)/(2*sigma2))),2.2);
      }
    }
    glTextureSubImage2D(impl->flare_tex, 0, 0, 0, 
      tex_size, tex_size, GL_RED, GL_FLOAT, pixels.data());
  }
}

void createVaosVbos(RendererImpl *impl)
{
  // Particle VAO
  glCreateVertexArrays(1, &impl->vao_particles);
  glCreateBuffers(1, &impl->vbo_particles_pos);
  glCreateBuffers(1, &impl->ssbo_velocities);
  glVertexArrayVertexBuffer(impl->vao_particles, 0, impl->vbo_particles_pos, 0, sizeof(glm::vec4));
  glVertexArrayVertexBuffer(impl->vao_particles, 1, impl->ssbo_velocities  , 0, sizeof(glm::vec4));

  // Position
  glEnableVertexArrayAttrib( impl->vao_particles, 0);
  glVertexArrayAttribFormat( impl->vao_particles, 0, 4, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(impl->vao_particles, 0, 0);

  // Velocity
  glEnableVertexArrayAttrib( impl->vao_particles, 1);
  glVertexArrayAttribFormat( impl->vao_particles, 1, 4, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(impl->vao_particles, 1, 1);

  // Deferred VAO
  glCreateVertexArrays(1, &impl->vao_deferred);
  glCreateBuffers(1, &impl->vbo_deferred);
  glVertexArrayVertexBuffer( impl->vao_deferred, 0, impl->vbo_deferred, 0, sizeof(glm::vec2));
  // Position
  glEnableVertexArrayAttrib( impl->vao_deferred, 0);
  glVertexArrayAttribFormat( impl->vao_deferred, 0, 2, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(impl->vao_deferred, 0, 0);

  // Deferred tri
  glm::vec2 tri[3] = {
    glm::vec2(-2,-1), 
    glm::vec2(+2,-1),
    glm::vec2( 0, 4)};
  glNamedBufferStorage(impl->vbo_deferred, 3*sizeof(glm::vec2), tri, 0);
}

void Renderer::populateParticles(const vector<glm::vec4> pos, const vector<glm::vec4> vel)
{
  // SSBO allocation & data upload
  glNamedBufferStorage(impl->vbo_particles_pos, pos.size()*sizeof(glm::vec4), pos.data(), 0);
  glNamedBufferStorage(impl->ssbo_velocities  , vel.size()*sizeof(glm::vec4), vel.data(), 0);

  // SSBO binding
  glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, impl->vbo_particles_pos, 
    0, sizeof(glm::vec4)*pos.size());
  glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, impl->ssbo_velocities,
    0, sizeof(glm::vec4)*vel.size());
}

void initShaders(RendererImpl *impl)
{
  impl->program_interaction.source(GL_COMPUTE_SHADER, "shaders/interaction.comp");
  impl->program_interaction.link();

  impl->program_integration.source(GL_COMPUTE_SHADER, "shaders/integration.comp");
  impl->program_integration.link();

  impl->program_hdr.source(GL_VERTEX_SHADER  , "shaders/main.vert");
  impl->program_hdr.source(GL_FRAGMENT_SHADER, "shaders/main.frag");
  impl->program_hdr.source(GL_GEOMETRY_SHADER, "shaders/main.geom");
  impl->program_hdr.link();

  impl->program_tonemap.source(GL_VERTEX_SHADER,  "shaders/deferred.vert");
  impl->program_tonemap.source(GL_FRAGMENT_SHADER,"shaders/tonemap.frag" );
  impl->program_tonemap.link();

  impl->program_blur.source(GL_VERTEX_SHADER,  "shaders/deferred.vert");
  impl->program_blur.source(GL_FRAGMENT_SHADER,"shaders/blur.frag"    );
  impl->program_blur.link();

  impl->program_lum.source(GL_VERTEX_SHADER  , "shaders/deferred.vert");
  impl->program_lum.source(GL_FRAGMENT_SHADER, "shaders/luminance.frag");
  impl->program_lum.link();
}

void initFbos(RendererImpl *impl)
{
  int blur_dsc = 2;
  impl->blur_downscale = blur_dsc;
  
  glCreateFramebuffers(4, impl->fbos);
  glCreateTextures(GL_TEXTURE_2D, 4, impl->attachs);

  int base_width  = impl->width +2*FBO_MARGIN;
  int base_height = impl->height+2*FBO_MARGIN;

  int widths [] = { base_width,
                    base_width/blur_dsc,
                    base_width/blur_dsc,
                    base_width/2};

  int heights[] = { base_height,
                    base_height/blur_dsc,
                    base_height/blur_dsc,
                    base_height/2};

  impl->lum_lod = (int)floor(log2(max(base_width,base_height)/2));
  int mipmaps[] = { 1, 1, 1, impl->lum_lod+1};
  GLenum types[] = {GL_RGBA16F, GL_RGBA16F, GL_RGBA16F, GL_R16F};
  GLenum min_filters[] = {GL_LINEAR, GL_LINEAR,GL_LINEAR,GL_LINEAR_MIPMAP_LINEAR};

  for (int i=0;i<4;++i)
  {
    glTextureStorage2D(impl->attachs[i], mipmaps[i], types[i], widths[i], heights[i]);
    glTextureParameteri(impl->attachs[i], GL_TEXTURE_MIN_FILTER, min_filters[i]);
    glTextureParameteri(impl->attachs[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(impl->attachs[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(impl->fbos[i], GL_COLOR_ATTACHMENT0, impl->attachs[i], 0);
  }
}

void setUniforms(RendererImpl *impl, sim_param params)
{
  // const Uniforms
  glProgramUniform1f(impl->program_interaction.getId(), 0, params.dt);
  glProgramUniform1f(impl->program_interaction.getId(), 1, params.G);
  glProgramUniform1f(impl->program_interaction.getId(), 2, params.damping);
  glProgramUniform1f(impl->program_integration.getId(), 0, params.dt);
  // NDC sprite size
  glProgramUniform2f(impl->program_hdr.getId(), 8, 
    impl->tex_size/float(2*impl->width), 
    impl->tex_size/float(2*impl->height));
  // Blur sample offset length
  glProgramUniform2f(impl->program_blur.getId(), 0, 
    (float)impl->blur_downscale/impl->width, 
    (float)impl->blur_downscale/impl->height);
}

void Renderer::stepSim(size_t num_particles)
{
  // Interaction step
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  glUseProgram(impl->program_interaction.getId());
  glDispatchCompute(num_particles/256, 1, 1);

  // Integration step
  glUseProgram(impl->program_integration.getId());
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  glDispatchCompute(num_particles/256, 1, 1);
}

void Renderer::render(size_t num_particles, glm::mat4 proj_mat, glm::mat4 view_mat)
{
  // Particle HDR rendering
  glViewport(0,0,impl->width+2*FBO_MARGIN, impl->height+2*FBO_MARGIN);
  glBindVertexArray(impl->vao_particles);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glBindFramebuffer(GL_FRAMEBUFFER, impl->fbos[0]);
  glUseProgram(impl->program_hdr.getId());
  glClear(GL_COLOR_BUFFER_BIT);
  glProgramUniformMatrix4fv(impl->program_hdr.getId(), 0, 1, GL_FALSE, glm::value_ptr(view_mat));
  glProgramUniformMatrix4fv(impl->program_hdr.getId(), 4, 1, GL_FALSE, glm::value_ptr(proj_mat));
  glBindTextureUnit(0, impl->flare_tex);
  glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
  glDrawArrays(GL_POINTS, 0, num_particles);

  glBindVertexArray(impl->vao_deferred);
  glDisable(GL_BLEND);

  glViewport(0,0,
    (impl->width +2*FBO_MARGIN)/impl->blur_downscale, 
    (impl->height+2*FBO_MARGIN)/impl->blur_downscale);
  glUseProgram(impl->program_blur.getId());

  // Blur pingpong (N horizontal blurs then N vertical blurs)
  int loop = 0;
  for (int i=0;i<2;++i)
  {
    if (i==0) glProgramUniform2f(impl->program_blur.getId(), 1, 1, 0);
    else      glProgramUniform2f(impl->program_blur.getId(), 1, 0, 1);
    for (int j=0;j<100;++j)
    {
      GLuint fbo = impl->fbos[(loop%2)+1];
      GLuint attach = impl->attachs[loop?((loop+1)%2+1):0];
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glBindTextureUnit(0, attach);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      loop++;
    }
  }

  // Average luminance
  glViewport(0,0,
    (impl->width +2*FBO_MARGIN)/2, 
    (impl->height+2*FBO_MARGIN)/2);
  glBindFramebuffer(GL_FRAMEBUFFER, impl->fbos[3]);
  glUseProgram(impl->program_lum.getId());
  glBindTextureUnit(0, impl->attachs[0]);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glGenerateTextureMipmap(impl->attachs[3]);

  // Tonemapping step (direct to screen)
  glViewport(0,0,impl->width,impl->height);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glUseProgram(impl->program_tonemap.getId());
  glProgramUniform1i(impl->program_tonemap.getId(), 0, impl->lum_lod);
  glBindTextureUnit(0, impl->attachs[0]);
  glBindTextureUnit(1, impl->attachs[2]);
  glBindTextureUnit(2, impl->attachs[3]);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}