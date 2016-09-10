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

  c.velocity *= 0.72; // damping

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

glm::mat4 camera_get_view(camera c)
{ 
  // polar to cartesian coordinates
  glm::vec3 view_pos = glm::vec3(
    cos(c.position.x)*cos(c.position.y), 
    sin(c.position.x)*cos(c.position.y), 
    sin(c.position.y))*c.position.z;

  return glm::lookAt(
    view_pos,
    glm::vec3(0,0,0),
    glm::vec3(0,0,1));
}