#pragma once

#include <glm/glm.hpp>

class Camera
{

public:
	Camera();

	/**
	 * Computes next step of camera parameters
	 * @param c camera at step n
	 * @return camera at step n+1
	 */
	void step();

	/**
	 * Computes projection matrix from camera parameters
	 * @param c camera parameters
	 * @param width viewport width
	 * @param height viewport height
	 * @return projection matrix
	 */
	glm::mat4 getProj(int width, int height);

	/**
	 * Computes view matrix from camera parameters
	 * @param c camera parameters
	 * @param view matrix
	 */
	glm::mat4 getView();

	glm::vec3 getForward();
	glm::vec3 getRight();
	glm::vec3 getUp();

	glm::vec3 getPosition();

	void addVelocity(glm::vec3 vel);
	void addLookAtVelocity(glm::vec3 vel);


private:
  glm::vec3 position;    ///< Polar coordinates in radians
  glm::vec3 velocity;    ///< dp/dt of polar coordinates
  glm::vec3 look_at;     ///< Where is the camera looking at
  glm::vec3 look_at_vel; ///< dp/dt of lookat position
};

