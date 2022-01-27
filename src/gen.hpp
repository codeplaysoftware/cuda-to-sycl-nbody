// Copyright (C) 2016 - 2018 Sarah Le Luron
// Copyright (C) 2022 Codeplay Software Limited

#pragma once
#include <glm/glm.hpp>
#include <vector>

/**
 * Generates a random particle position
 * @return 3D position + w component at 1.f
 */
glm::vec4 randomParticlePos();

/**
 * Generates a random particle velocity
 * @param pos the same particle's position
 * @return 3D velocity + w component at 0.f
 */
glm::vec4 randomParticleVel(glm::vec4 pos);

std::vector<float> genFlareTex(int size);