#include "gen.hpp"

#include <random>

const float PI = 3.14159265358979323846;

// Copyright (C) 2016 - 2018 Sarah Le Luron
// Copyright (C) 2022 Codeplay Software Limited

using namespace std;

mt19937 rng;
uniform_real_distribution<> dis(0, 1);

glm::vec4 randomParticlePos() {
  // Random position on a 'thick disk'
  glm::vec4 particle;
  float t = dis(rng) * 2 * PI;
  float s = dis(rng) * 100;
  particle.x = cos(t) * s;
  particle.y = sin(t) * s;
  particle.z = dis(rng) * 4;

  particle.w = 1.f;
  return particle;
}

glm::vec4 randomParticleVel(glm::vec4 pos) {
  // Initial velocity is 'orbital' velocity from position
  glm::vec3 vel = glm::cross(glm::vec3(pos), glm::vec3(0, 0, 1));
  float orbital_vel = sqrt(2.0 * glm::length(vel));
  vel = glm::normalize(vel) * orbital_vel;
  return glm::vec4(vel, 0.0);
}

std::vector<float> genFlareTex(int tex_size) {
  std::vector<float> pixels(tex_size * tex_size);
  float sigma2 = tex_size / 2.0;
  float A = 1.0;
  for (int i = 0; i < tex_size; ++i) {
    float i1 = i - tex_size / 2;
    for (int j = 0; j < tex_size; ++j) {
      float j1 = j - tex_size / 2;
      // gamma corrected gauss
      pixels[i * tex_size + j] = pow(
          A * exp(-((i1 * i1) / (2 * sigma2) + (j1 * j1) / (2 * sigma2))),
          2.2);
    }
  }
  return pixels;
}
