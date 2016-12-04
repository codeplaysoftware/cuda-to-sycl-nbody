#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

const float PI = 3.14159265358979323846;

using namespace std;

Camera::Camera()
{
	position.x = 0;
	position.y = PI/4;
	position.z = 50.0;
}

void Camera::step()
{
	position.x -= velocity.x;
	position.y -= velocity.y;
	position.z *= (1.0-velocity.z);
	look_at += look_at_vel;

	velocity *= 0.72; // damping
	look_at_vel *= 0.90;

	// limits
	if (position.x < 0)       position.x += 2*PI;
	if (position.x >= 2*PI) position.x -= 2*PI;
	position.y = max(-(float)PI/2+0.001f, min(position.y, (float)PI/2-0.001f));
}

glm::mat4 Camera::getProj(int width, int height)
{
	return glm::infinitePerspective(
		glm::radians(30.0f),width/(float)height, 1.f);
}

glm::vec3 getCartesianCoordinates(glm::vec3 v)
{
	return glm::vec3(
		cos(v.x)*cos(v.y), 
		sin(v.x)*cos(v.y), 
		sin(v.y))*v.z;
}

glm::mat4 Camera::getView()
{ 
	// polar to cartesian coordinates
	glm::vec3 view_pos = getCartesianCoordinates(position);

	return glm::lookAt(
		view_pos+look_at,
		look_at,
		glm::vec3(0,0,1));
}

glm::vec3 Camera::getForward()
{
	return glm::normalize(-getCartesianCoordinates(position));
}

glm::vec3 Camera::getRight()
{
	return glm::normalize(
		glm::cross(
			getCartesianCoordinates(position),
			glm::vec3(0,0,1)));
}

glm::vec3 Camera::getUp()
{
	return glm::normalize(
		glm::cross(
			getCartesianCoordinates(position),
			getRight()));
}

void Camera::addVelocity(glm::vec3 vel)
{
	velocity += vel;
}

void Camera::addLookAtVelocity(glm::vec3 vel)
{
	look_at_vel += vel;
}

glm::vec3 Camera::getPosition()
{
	return position;
}

