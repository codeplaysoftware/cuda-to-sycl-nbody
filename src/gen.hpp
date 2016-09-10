#pragma once
#include <glm/glm.hpp>

/**
 * Generates a random particle position
 * @return 3D position + w component at 1.f
 */
glm::vec4 random_particle_pos();

/**
 * Generates a random particle velocity
 * @param pos the same particle's position
 * @return 3D velocity + w component at 0.f
 */
glm::vec4 random_particle_vel(glm::vec4 pos);