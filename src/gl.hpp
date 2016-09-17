#pragma once

#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>

#include "sim_param.hpp"

/**
 * Contains the gl state necessary to run the nbody simulation
 */
struct gl_state
{
  GLuint flare_tex;           ///< Texture for the star flare
  GLuint vao_particles;       ///< Vertex definition for points
  GLuint vbo_particles_pos;   ///< Particle position buffer
  GLuint ssbo_velocities;     ///< Particle velocity buffer
  GLuint vao_deferred;        ///< Vertex definition for deferred
  GLuint vbo_deferred;        ///< Vertex buffer of deferred fullscreen tri

  /** Shader programs **/
  GLuint program_interaction; ///< Gravity interaction step
  GLuint program_integration; ///< Position integration step
  GLuint program_hdr;         ///< HDR rendering step
  GLuint program_blur;        ///< Bloom blurring step
  GLuint program_lum;         ///< Average luminance step
  GLuint program_tonemap;     ///< Tonemapping step

  GLuint fbos[4];             ///< FBOs (0 for hdr, 1 & 2 for blur ping pong, 3 for luminance)
  GLuint attachs[4];          ///< Respective FBO attachments.

  int tex_size;               ///< Flare texture size in pixels
  int lum_lod;                ///< Luminance texture level to sample from
  int blur_downscale;         ///< Downscale factor for the blurring step
  int width;                  ///< Viewport width
  int height;                 ///< Viewport height
};

/**
 * Initializes the gl state
 * @param state
 * @param width viewport width
 * @param height viewport height
 * @param params simulation parameters
 */
void init(gl_state &state, int width, int height, sim_param params);

/**
 * Supplies the gl state with initial particle position and velocity
 * @param state
 * @param pos particle positions
 * @param vel particle velocities
 */
void populate_particles(gl_state &state, 
  const std::vector<glm::vec4> pos, 
  const std::vector<glm::vec4> vel);

/**
 * Steps the simulation once, with the parameters provided with @see init
 * @param state
 * @param num_particles number of particles to simulate
 */
void step_sim(gl_state &state, size_t num_particles);

/**
 * Renders the particles at the current step
 * @param state
 * @param num_particles number of particles to render
 * @param proj_mat projection matrix @see camera_get_proj
 * @param view_mat view matrix @see camera_get_view
 */
void render(gl_state &state, size_t num_particles, glm::mat4 proj_mat, glm::mat4 view_mat);