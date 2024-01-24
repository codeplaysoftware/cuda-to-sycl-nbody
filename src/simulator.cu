// Copyright (C) 2022 Codeplay Software Limited
// This work is licensed under the terms of the MIT license.
// For a copy, see https://opensource.org/licenses/MIT.

#include "simulator.cuh"
//#include <cstddef>
#include <stdio.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <tuple>
#include <chrono>
#include <iostream>

namespace simulation {

  // Forward decl
  __global__ void particle_interaction_b(ParticleData_d pPos,
      ParticleData_d pNextPos,
      ParticleData_d pVel, SimParam params);

  __global__ void particle_interaction_nb(ParticleData_d pPos,
      ParticleData_d pNextPos,
      ParticleData_d pVel, SimParam params);

  DiskGalaxySimulator::DiskGalaxySimulator(SimParam params_)
    : params(params_),
    pos(params_.numParticles),
    vel(params_.numParticles),
    pos_d(params_.numParticles),
    vel_d(params_.numParticles),
    pos_next_d(params_.numParticles) {
      randomParticlePos();
      initialParticleVel();
      sendToDevice();
    };

  const std::string* DiskGalaxySimulator::getDeviceName() {
    // Query the device first time only
    if(devName.empty()){
      char devNameHolder[256];
      int error_id = cuDeviceGetName(devNameHolder, 256, 0); // Assume main device
      if(error_id != CUDA_SUCCESS) devName = "Unknown Device";
      else devName = devNameHolder;
    }
    return &devName;
  }

  void DiskGalaxySimulator::stepSim() {
    // Compute updated positions
    int wg_size = getGwSize();
    int nblocks = ((getNumParticles() - 1) / wg_size) + 1;

    // Profiling info - rather than using the CUDA event recording
    // approach, we are instead measuring the time from before kernel
    // submission until host synchronization. This is more portable via
    // dpct.
    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < params.simIterationsPerFrame; i++) {
      if ( getUseBranch() ) {
        particle_interaction_b<<<nblocks, wg_size>>>(pos_d, pos_next_d, vel_d,
            params);
      } else {
        particle_interaction_nb<<<nblocks, wg_size>>>(pos_d, pos_next_d, vel_d,
            params);
      }
      std::swap(pos_d, pos_next_d);
    }
    gpuErrchk(cudaDeviceSynchronize());
    auto stop = std::chrono::steady_clock::now();
    lastStepTime =
      std::chrono::duration<float, std::milli>(stop - start)
      .count();

    // Sync data
    recvFromDevice();
  }

  // Only necessary because we can't initialize data on device yet, in a
  // dpct-friendly way
  void DiskGalaxySimulator::sendToDevice() {
    gpuErrchk(cudaDeviceSynchronize());

    gpuErrchk(cudaMemcpy(pos_d.x, pos.x.data(),
          params.numParticles * sizeof(coords_t),
          cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(pos_d.y, pos.y.data(),
          params.numParticles * sizeof(coords_t),
          cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(pos_d.z, pos.z.data(),
          params.numParticles * sizeof(coords_t),
          cudaMemcpyHostToDevice));

    gpuErrchk(cudaMemcpy(vel_d.x, vel.x.data(),
          params.numParticles * sizeof(coords_t),
          cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(vel_d.y, vel.y.data(),
          params.numParticles * sizeof(coords_t),
          cudaMemcpyHostToDevice));
    gpuErrchk(cudaMemcpy(vel_d.z, vel.z.data(),
          params.numParticles * sizeof(coords_t),
          cudaMemcpyHostToDevice));

    gpuErrchk(cudaDeviceSynchronize());
  }

  // Receive particle positions & velocity from device
  void DiskGalaxySimulator::recvFromDevice() {
    gpuErrchk(cudaDeviceSynchronize());

    gpuErrchk(cudaMemcpy(pos.x.data(), pos_d.x,
          params.numParticles * sizeof(coords_t),
          cudaMemcpyDeviceToHost));
    gpuErrchk(cudaMemcpy(pos.y.data(), pos_d.y,
          params.numParticles * sizeof(coords_t),
          cudaMemcpyDeviceToHost));
    gpuErrchk(cudaMemcpy(pos.z.data(), pos_d.z,
          params.numParticles * sizeof(coords_t),
          cudaMemcpyDeviceToHost));

    gpuErrchk(cudaMemcpy(vel.x.data(), vel_d.x,
          params.numParticles * sizeof(coords_t),
          cudaMemcpyDeviceToHost));
    gpuErrchk(cudaMemcpy(vel.y.data(), vel_d.y,
          params.numParticles * sizeof(coords_t),
          cudaMemcpyDeviceToHost));
    gpuErrchk(cudaMemcpy(vel.z.data(), vel_d.z,
          params.numParticles * sizeof(coords_t),
          cudaMemcpyDeviceToHost));
    gpuErrchk(cudaDeviceSynchronize());
  }

  void DiskGalaxySimulator::randomParticlePos() {
    // deterministic - default seed
    std::mt19937 gen;
    std::uniform_real_distribution<> dis(0.0, 1.0);

    // Disk shape in x-y plane
    for (int i = 0; i < params.numParticles; i++) {
      float t = dis(gen) * 2 * PI;
      float s = dis(gen) * 100;
      pos.x[i] = cos(t) * s;
      pos.y[i] = sin(t) * s;
    }

    // Z component is independent (uniform range 0-4)
    std::generate(begin(pos.z), end(pos.z),
        [&gen, &dis]() { return 4.0 * dis(gen); });
  }

  void DiskGalaxySimulator::initialParticleVel() {
    for (int i = 0; i < params.numParticles; i++) {
      vec3 vel = cross({pos.x[i], pos.y[i], pos.z[i]}, {0.0, 0.0, 1.0});
      coords_t orbital_vel = std::sqrt(2.0 * length(vel));
      vel = normalize(vel) * orbital_vel;
      this->vel.x[i] = vel.x;
      this->vel.y[i] = vel.y;
      this->vel.z[i] = vel.z;
    }
  }

  const ParticleData& DiskGalaxySimulator::getParticlePos() { return pos; };

  const ParticleData& DiskGalaxySimulator::getParticleVel() { return vel; };

  // Linear Algebra functions (not yet exposed in header)
  HOSTDEV vec3 cross(const vec3 v0, const vec3 v1) {
    return vec3(v0.y * v1.z - v0.z * v1.y, v0.z * v1.x - v0.x * v1.z,
        v0.x * v1.y - v0.y * v1.x);
  };

  HOSTDEV coords_t length(const vec3 v) {
    return std::sqrt(std::pow(v.x, 2) + std::pow(v.y, 2) + std::pow(v.z, 2));
  }

  HOSTDEV vec3 normalize(const vec3 v) {
    vec3 result = v;
    coords_t len = length(v);
    result.x /= len;
    result.y /= len;
    result.z /= len;
    return result;
  }

  /* O(n^2) implementation (no distance threshold), with no shared
     memory etc.
   */
  __global__ void particle_interaction_b(ParticleData_d pPos,
      ParticleData_d pNextPos,
      ParticleData_d pVel, SimParam params) {
    int id = threadIdx.x + (blockIdx.x * blockDim.x);
    if (id >= params.numParticles) return;

    vec3 force(0.0f, 0.0f, 0.0f);
    vec3 pos(pPos.x[id], pPos.y[id], pPos.z[id]);

#pragma unroll 4
    for (int i = 0; i < params.numParticles; i++) {
      vec3 other_pos{pPos.x[i], pPos.y[i], pPos.z[i]};
      vec3 r = other_pos - pos;
      // Fast computation of 1/(|r|^3)
      coords_t dist_sqr = dot(r, r) + params.distEps;
      coords_t inv_dist_cube = rsqrt(dist_sqr * dist_sqr * dist_sqr);

      // assume uniform unit mass
      if ( i == id ) continue;

      force += r * inv_dist_cube;
      //         force += r * inv_dist_cube * (i == id);
    }

    // Update velocity
    vec3 curr_vel(pVel.x[id], pVel.y[id], pVel.z[id]);
    curr_vel *= params.damping;
    curr_vel += force * params.dt * params.G;

    pVel.x[id] = curr_vel.x;
    pVel.y[id] = curr_vel.y;
    pVel.z[id] = curr_vel.z;

    // Update position (integration)
    vec3 curr_pos(pPos.x[id], pPos.y[id], pPos.z[id]);

    curr_pos += curr_vel * params.dt;
    pNextPos.x[id] = curr_pos.x;
    pNextPos.y[id] = curr_pos.y;
    pNextPos.z[id] = curr_pos.z;
  }

  __global__ void particle_interaction_nb(ParticleData_d pPos,
      ParticleData_d pNextPos,
      ParticleData_d pVel, SimParam params) {
    int id = threadIdx.x + (blockIdx.x * blockDim.x);
    if (id >= params.numParticles) return;

    vec3 force(0.0f, 0.0f, 0.0f);
    vec3 pos(pPos.x[id], pPos.y[id], pPos.z[id]);

#pragma unroll 4
    for (int i = 0; i < params.numParticles; i++) {
      vec3 other_pos{pPos.x[i], pPos.y[i], pPos.z[i]};
      vec3 r = other_pos - pos;
      // Fast computation of 1/(|r|^3)
      coords_t dist_sqr = dot(r, r) + params.distEps;
      coords_t inv_dist_cube = rsqrt(dist_sqr * dist_sqr * dist_sqr);

      // assume uniform unit mass
      force += r * inv_dist_cube * (i == id);
    }

    // Update velocity
    vec3 curr_vel(pVel.x[id], pVel.y[id], pVel.z[id]);
    curr_vel *= params.damping;
    curr_vel += force * params.dt * params.G;

    pVel.x[id] = curr_vel.x;
    pVel.y[id] = curr_vel.y;
    pVel.z[id] = curr_vel.z;

    // Update position (integration)
    vec3 curr_pos(pPos.x[id], pPos.y[id], pPos.z[id]);

    curr_pos += curr_vel * params.dt;
    pNextPos.x[id] = curr_pos.x;
    pNextPos.y[id] = curr_pos.y;
    pNextPos.z[id] = curr_pos.z;
  }
}  // namespace simulation
