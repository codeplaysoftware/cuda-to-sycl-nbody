#pragma once

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <stdio.h>

#include <vector>

#include "sim_param.hpp"

#ifdef __CUDACC__
#define HOSTDEV __host__ __device__
#else
#define HOSTDEV
#endif

#define gpuErrchk(ans) \
   { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line,
                      bool abort = true) {
   if (code != cudaSuccess) {
      fprintf(stderr, "GPUassert: %s %s %d\n", cudaGetErrorString(code), file,
              line);
      if (abort) exit(code);
   }
}

namespace simulation {

   const float PI = 3.14159265358979323846;

   typedef float coords_t;

   struct vec3 {
      coords_t x;
      coords_t y;
      coords_t z;

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
      coords_t *x;
      coords_t *y;
      coords_t *z;

      ParticleData_d() = delete;
      ParticleData_d(size_t n) {
         // Allocate device memory for particle coords & velocity...
         gpuErrchk(cudaMalloc((void **)&x, sizeof(coords_t) * n));
         gpuErrchk(cudaMalloc((void **)&y, sizeof(coords_t) * n));
         gpuErrchk(cudaMalloc((void **)&z, sizeof(coords_t) * n));
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
      // DiskGalaxySimulator() = delete; // no default ctor
      DiskGalaxySimulator(SimParam params_);

      void stepSim();
      size_t getNumParticles() { return params.numParticles; }
      const ParticleData &getParticlePos();
      const ParticleData &getParticleVel();

     private:
      SimParam params;

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