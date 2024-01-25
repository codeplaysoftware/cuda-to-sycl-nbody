// Copyright (C) 2022 Codeplay Software Limited
// This work is licensed under the terms of the MIT license.
// For a copy, see https://opensource.org/licenses/MIT.

#include <sycl/sycl.hpp>
#include <dpct/dpct.hpp>
#include "simulator.dp.hpp"
//#include <cstddef>
#include <stdio.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <tuple>
#include <chrono>

namespace simulation {

  // Forward decl
  void particle_interaction_b(ParticleData_d pPos,
      ParticleData_d pNextPos,
      ParticleData_d pVel, SimParam params,
      const sycl::nd_item<1> &item_ct1);

  void particle_interaction_nb(ParticleData_d pPos,
      ParticleData_d pNextPos,
      ParticleData_d pVel, SimParam params,
      const sycl::nd_item<1> &item_ct1);

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
      /*
DPCT1003:4: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
       */
      int error_id = (memcpy(devNameHolder,
            dpct::dev_mgr::instance()
            .get_device(0)
            .get_info<sycl::info::device::name>()
            .c_str(),
            256),
          0);  // Assume main device
      if (error_id != 0) devName = "Unknown Device";
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
      dpct::get_default_queue().submit([&](sycl::handler &cgh) {
          auto pos_d_ct0 = pos_d;
          auto pos_next_d_ct1 = pos_next_d;
          auto vel_d_ct2 = vel_d;
          auto params_ct3 = params;

          if ( getUseBranch() ) {
          cgh.parallel_for<
          dpct_kernel_name<class particle_interaction_da5588>>(
              sycl::nd_range<1>(
                sycl::range<1>(nblocks) * sycl::range<1>(wg_size),
                sycl::range<1>(wg_size)),
              [=](sycl::nd_item<1> item_ct1) {
              particle_interaction_b(pos_d_ct0, pos_next_d_ct1, vel_d_ct2,
                  params_ct3, item_ct1);
              });
          } else {
          cgh.parallel_for<
          dpct_kernel_name<class particle_interaction_da5589>>(
              sycl::nd_range<1>(
                sycl::range<1>(nblocks) * sycl::range<1>(wg_size),
                sycl::range<1>(wg_size)),
              [=](sycl::nd_item<1> item_ct1) {
              particle_interaction_nb(pos_d_ct0, pos_next_d_ct1, vel_d_ct2,
                  params_ct3, item_ct1);
              });
          }
      });
      std::swap(pos_d, pos_next_d);
    }
    /*
DPCT1003:5: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((dpct::get_current_device().queues_wait_and_throw(), 0));
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
    dpct::device_ext &dev_ct1 = dpct::get_current_device();
    sycl::queue &q_ct1 = dev_ct1.default_queue();
    /*
DPCT1003:6: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((dev_ct1.queues_wait_and_throw(), 0));

    /*
DPCT1003:7: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(pos_d.x, pos.x.data(),
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));
    /*
DPCT1003:8: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(pos_d.y, pos.y.data(),
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));
    /*
DPCT1003:9: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(pos_d.z, pos.z.data(),
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));

    /*
DPCT1003:10: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(vel_d.x, vel.x.data(),
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));
    /*
DPCT1003:11: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(vel_d.y, vel.y.data(),
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));
    /*
DPCT1003:12: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(vel_d.z, vel.z.data(),
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));

    /*
DPCT1003:13: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((dev_ct1.queues_wait_and_throw(), 0));
  }

  // Receive particle positions & velocity from device
  void DiskGalaxySimulator::recvFromDevice() {
    dpct::device_ext &dev_ct1 = dpct::get_current_device();
    sycl::queue &q_ct1 = dev_ct1.default_queue();
    /*
DPCT1003:14: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((dev_ct1.queues_wait_and_throw(), 0));

    /*
DPCT1003:15: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(pos.x.data(), pos_d.x,
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));
    /*
DPCT1003:16: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(pos.y.data(), pos_d.y,
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));
    /*
DPCT1003:17: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(pos.z.data(), pos_d.z,
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));

    /*
DPCT1003:18: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(vel.x.data(), vel_d.x,
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));
    /*
DPCT1003:19: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(vel.y.data(), vel_d.y,
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));
    /*
DPCT1003:20: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((q_ct1
          .memcpy(vel.z.data(), vel_d.z,
            params.numParticles * sizeof(coords_t))
          .wait(),
          0));
    /*
DPCT1003:21: Migrated API does not return error code. (*, 0) is inserted.
You may need to rewrite this code.
     */
    gpuErrchk((dev_ct1.queues_wait_and_throw(), 0));
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
    return sycl::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
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
  void particle_interaction_b(ParticleData_d pPos,
      ParticleData_d pNextPos,
      ParticleData_d pVel, SimParam params,
      const sycl::nd_item<1> &item_ct1) {
    int id = item_ct1.get_local_id(0) +
      (item_ct1.get_group(0) * item_ct1.get_local_range(0));
    if (id >= params.numParticles) return;

    vec3 force(0.0f, 0.0f, 0.0f);
    vec3 pos(pPos.x[id], pPos.y[id], pPos.z[id]);

#pragma unroll 4
    for (int i = 0; i < params.numParticles; i++) {
      vec3 other_pos{pPos.x[i], pPos.y[i], pPos.z[i]};
      vec3 r = other_pos - pos;
      // Fast computation of 1/(|r|^3)
      coords_t dist_sqr = dot(r, r) + params.distEps;
      coords_t inv_dist_cube = sycl::rsqrt(dist_sqr * dist_sqr * dist_sqr);

      // assume uniform unit mass
      if (i == id) continue;

      force += r * inv_dist_cube;
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

  void particle_interaction_nb(ParticleData_d pPos,
      ParticleData_d pNextPos,
      ParticleData_d pVel, SimParam params,
      const sycl::nd_item<1> &item_ct1) {
    int id = item_ct1.get_local_id(0) +
      (item_ct1.get_group(0) * item_ct1.get_local_range(0));
    if (id >= params.numParticles) return;

    vec3 force(0.0f, 0.0f, 0.0f);
    vec3 pos(pPos.x[id], pPos.y[id], pPos.z[id]);

#pragma unroll 4
    for (int i = 0; i < params.numParticles; i++) {
      vec3 other_pos{pPos.x[i], pPos.y[i], pPos.z[i]};
      vec3 r = other_pos - pos;
      // Fast computation of 1/(|r|^3)
      coords_t dist_sqr = dot(r, r) + params.distEps;
      coords_t inv_dist_cube = sycl::rsqrt(dist_sqr * dist_sqr * dist_sqr);

      // assume uniform unit mass
      force += r * inv_dist_cube * ( i != id );
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
