#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

using namespace std;

camera new_camera()
{
  camera c;
  c.position.x = 0;
  c.position.y = M_PI/4;
  c.position.z = 50.0;
  return c;
}

camera camera_step(camera c)
{
  c.position.x -= c.velocity.x;
  c.position.y -= c.velocity.y;
  c.position.z *= (1.0-c.velocity.z);
  c.look_at += c.look_at_vel;

  c.velocity *= 0.72; // damping
  c.look_at_vel *= 0.90;

  // limits
  if (c.position.x < 0)       c.position.x += 2*M_PI;
  if (c.position.x >= 2*M_PI) c.position.x -= 2*M_PI;
  c.position.y = max(-(float)M_PI/2+0.001f, min(c.position.y, (float)M_PI/2-0.001f));

  return c;
}

glm::mat4 camera_get_proj(camera c, int width, int height)
{
  return glm::infinitePerspective(
    glm::radians(30.0f),width/(float)height, 1.f);
}

glm::vec3 get_cartesian_coordinates(glm::vec3 v)
{
  return glm::vec3(
    cos(v.x)*cos(v.y), 
    sin(v.x)*cos(v.y), 
    sin(v.y))*v.z;
}

glm::mat4 camera_get_view(camera c)
{ 
  // polar to cartesian coordinates
  glm::vec3 view_pos = get_cartesian_coordinates(c.position);

  return glm::lookAt(
    view_pos+c.look_at,
    c.look_at,
    glm::vec3(0,0,1));
}

glm::vec3 camera_get_forward(camera c)
{
  return glm::normalize(-get_cartesian_coordinates(c.position));
}

glm::vec3 camera_get_right(camera c)
{
  return glm::normalize(
    glm::cross(
      get_cartesian_coordinates(c.position),
      glm::vec3(0,0,1)));
}

glm::vec3 camera_get_up(camera c)
{
  return glm::normalize(
    glm::cross(
      get_cartesian_coordinates(c.position),
      camera_get_right(c)));
}