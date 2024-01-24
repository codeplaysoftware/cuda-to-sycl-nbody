// Copyright (C) 2022 Codeplay Software Limited
// This work is licensed under the terms of the MIT license.
// For a copy, see https://opensource.org/licenses/MIT.

#pragma once

#include <sycl/sycl.hpp>
#include <dpct/dpct.hpp>
#include <stdio.h>

#include <string>
#include <vector>

#include "sim_param.hpp"

#ifdef SYCL_LANGUAGE_VERSION
#define HOSTDEV 
#else
#define HOSTDEV
#endif

#define gpuErrchk(ans) \
{ gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(dpct::err0 code, const char *file, int line,
    bool abort = true) {
}

namespace simulation {

  const float PI = 3.14159265358979323846;

  typedef float coords_t;

  struct vec3 {
    coords_t x = 0.0;
    coords_t y = 0.0;
    coords_t z = 0.0;

    HOSTDEV vec3() {};
    HOSTDEV vec3(coords_t x_, coords_t y_, coords_t z_)
      : x{x_}, y{y_}, z{z_} {}

    HOSTDEV inline vec3 &operator+=(const vec3 &rhs) {
      x += rhs.x;
      y += rhs.y;
      z += rhs.z;
      return *this;
    }

    HOSTDEV inline vec3 &operator*=(const coords_t &scale) {
      x *= scale;
      y *= scale;
      z *= scale;
      return *this;
    }
  };

  HOSTDEV inline const vec3 operator*(const vec3 &pos, const coords_t &scale) {
    return {pos.x * scale, pos.y * scale, pos.z * scale};
  }

  HOSTDEV inline const vec3 operator-(const vec3 &vec1, const vec3 &vec2) {
    return {vec1.x - vec2.x, vec1.y - vec2.y, vec1.z - vec2.z};
  }

  HOSTDEV inline coords_t dot(const vec3 &vec1, const vec3 &vec2) {
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
  }

  struct ParticleData {
    std::vector<coords_t> x;
    std::vector<coords_t> y;
    std::vector<coords_t> z;

    ParticleData(std::vector<coords_t> x_, std::vector<coords_t> y_,
        std::vector<coords_t> z_)
      : x(std::move(x_)), y(std::move(y_)), z(std::move(z_)){};
    ParticleData(size_t n) : x(n, 0.0), y(n, 0.0), z(n, 0.0){};
  };

  // Simply holds 3 coords_t* as a SoA
  struct ParticleData_d {
    coords_t *x = nullptr;
    coords_t *y = nullptr;
    coords_t *z = nullptr;

    ParticleData_d(size_t n) {
      dpct::device_ext &dev_ct1 = dpct::get_current_device();
      sycl::queue &q_ct1 = dev_ct1.default_queue();
      // Allocate device memory for particle coords & velocity...
      /*
DPCT1003:1: Migrated API does not return error code. (*, 0) is
inserted. You may need to rewrite this code.
       */
      gpuErrchk((x = sycl::malloc_device<coords_t>(n, q_ct1), 0));
      /*
DPCT1003:2: Migrated API does not return error code. (*, 0) is
inserted. You may need to rewrite this code.
       */
      gpuErrchk((y = sycl::malloc_device<coords_t>(n, q_ct1), 0));
      /*
DPCT1003:3: Migrated API does not return error code. (*, 0) is
inserted. You may need to rewrite this code.
       */
      gpuErrchk((z = sycl::malloc_device<coords_t>(n, q_ct1), 0));
    };
  };

  HOSTDEV coords_t length(const vec3 v);
  HOSTDEV vec3 cross(const vec3 v0, const vec3 v1);
  HOSTDEV vec3 normalize(const vec3 v);

  /*
     Interface class for Simulator
   */
  class Simulator {
    public:
      virtual void stepSim() = 0;
      virtual size_t getNumParticles() = 0;
      virtual const ParticleData &getParticlePos() = 0;
      virtual const ParticleData &getParticleVel() = 0;
      virtual float getLastStepTime() = 0;
      virtual const std::string* getDeviceName() = 0;
      virtual bool getUseBranch() = 0;
  };

  /*
     DiskGalaxySimulator class to handle execution of the nbody simulation.

     Regular data transfer only occurs in the device->host direction (from
     Simulator to Renderer).

Invariants:
- Has params
- Has valid particle positions & velocities, allocated on host & device
   */

  class DiskGalaxySimulator : public Simulator {
    public:
      DiskGalaxySimulator(SimParam params_);

      void stepSim();
      float getLastStepTime() { return lastStepTime; }
      size_t getNumParticles() { return params.numParticles; }
      const ParticleData &getParticlePos();
      const ParticleData &getParticleVel();
      const std::string* getDeviceName();
      int getGwSize() { return params.gwSize; }
      bool getUseBranch() { return params.useBranch; }

    private:
      SimParam params;
      std::string devName;
      float lastStepTime{0.0};

      // Data for particle positions & vel on host
      ParticleData pos;
      ParticleData vel;

      // and on device
      ParticleData_d pos_d;
      ParticleData_d pos_next_d;  // double buffering
      ParticleData_d vel_d;

      void randomParticlePos();
      void initialParticleVel();
      void sendToDevice();
      void recvFromDevice();
  };

}  // namespace simulation
