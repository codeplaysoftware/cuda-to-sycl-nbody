#include "gen.hpp"

#include <random>

using namespace std;

mt19937 rng;
uniform_real_distribution<> dis(0, 1);

glm::vec4 random_particle_pos()
{
  // Random position on a 'thick disk'
  glm::vec4 particle;
  float t = dis(rng)*2*M_PI;
  float s = dis(rng)*100;
  particle.x = cos(t)*s;
  particle.y = sin(t)*s;
  particle.z = dis(rng)*4;

  particle.w = 1.f;
  return particle;
}

glm::vec4 random_particle_vel(glm::vec4 pos)
{
  // Initial velocity depends on position
  glm::vec3 vel = glm::cross(glm::vec3(pos),glm::vec3(0,0,1));
  float orbital_vel = sqrt(2.0*glm::length(vel));
  vel = glm::normalize(vel)*orbital_vel;
  return glm::vec4(vel,0.0);
}