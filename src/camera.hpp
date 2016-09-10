#pragma once

#include <glm/glm.hpp>

struct camera
{
  glm::vec3 position; ///< Polar coordinates in radians
  glm::vec3 velocity; ///< dp/dt of polar coordinates
};

/**
 * Creates a new camera with default parameters
 * @return created camera
 */
camera new_camera();

/**
 * Computes next step of camera parameters
 * @param c camera at step n
 * @return camera at step n+1
 */
camera camera_step(camera c);

/**
 * Computes projection matrix from camera parameters
 * @param c camera parameters
 * @param width viewport width
 * @param height viewport height
 * @return projection matrix
 */
glm::mat4 camera_get_proj(camera c, int width, int height);

/**
 * Computes view matrix from camera parameters
 * @param c camera parameters
 * @param view matrix
 */
glm::mat4 camera_get_view(camera c);