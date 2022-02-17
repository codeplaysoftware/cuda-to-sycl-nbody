# nbody
CUDA N-body sim with OpenGL graphics & automatically CUDA->SYCL conversion using [dpct](https://www.intel.com/content/www/us/en/developer/tools/oneapi/dpc-compatibility-tool.html)

![](http://i.imgur.com/drzi33P.jpg)

Forked from https://github.com/salel/nbody

## Building

Build scripts for both versions (CUDA & SYCL) are located in `./scripts/`

Dependencies:
 - cmake
 - GLM
 - GLFW
 - GLEW

With apt:
```
sudo apt update
sudo apt install libglew-dev libglfw3-dev libglm-dev libxxf86vm-dev libxcursor-dev libxinerama-dev libxi-dev
```
Remember that you need an OpenGL 4.5 ready graphics card. Radeon 7xxx or Geforce GTX 4xx. No OSX. Maybe Intel.


### Converting CUDA to SYCL

The script ./scripts/run_dpct.sh calls a containerized version of the DPC++ Compatibility Tool to automatically convert the CUDA components of this project into SYCL. A docker container was used because the dev machine has an incompatible version of the CUDA driver. This should be adapted based on your environment. 

The DPC++ compatibility tool offers options for intercepting complex builds, but current dev environment restrictions require me to run dpct inside a docker container. This complicates things, so for now I'm just doing single source conversion on the simulator.cu file.

### CMake

The option `-DBUILD_SYCL` switches between building the CUDA version & the SYCL version of the code.
When compiling SYCL with `dpcpp`, `MKL_DIR` can be set to point to the `MKLConfig.cmake` file, this defaults to `/opt/intel/oneapi/mkl/latest/lib/cmake/mkl`.
Environment variables must be set by sourcing `setvars.sh` located at `/opt/intel/oneapi/setvars.sh`.

## Passing data between OpenGL & CUDA/SYCL

OpenGL & CUDA are capable of interoperating to share device memory, but this will not play well with the DPC++ Compatibility Tool. Instead, computed particle positions are migrated back to the host by CUDA/SYCL, then sent *back* to OpenGL via mapping.


## Simulation

The `DiskGalaxySimulator` class handles the physics of n-body interaction. The computation of interparticle forces, velocity & updated particle positions are handled by the CUDA kernel `particle_interaction`.

The equation solved by this code is equivalent to Eq. 1 [here](http://www.scholarpedia.org/article/N-body_simulations_(gravitational)), with the simplifying assumption that all particles have unit mass and there is no external/background force. This becomes:

![Eq1](/docs/Eq1.png)

The force vector on each particle (F) is the sum of gravitational forces from all other particles. For each particle interaction, the attractive force is inversely proportional to the square distance between them. This force is equal to the gravitational constant (`G`) multiplied by the unit vector pointing between the particles, divided by the square of this distance. The equation above has this last term slightly rearranged to avoid unnecessary computation.

Given the assumption of unit mass, the force (F) is equal to the acceleration, and so at each timestep, the force vector F is simpy added to the velocity vector. The position of each particle is updated by the velocity multiplied by the timestep size (`dt`).

A drag factor (`damping`) is used to regulate the velocity. At each timestep, the velocity is multiplied by the drag term, slowing the particles. The maximum force between very close particles is also limited for stability; this is achieved via an epsilon term (`distEps`) which is added to the distance between each particle pairing.

The `parameters` described in this section can all be adjusted via command line arguments, as follows:

`./nbodygl numParticles maxIterationsPerFrame damping dt distEps G`

Note that `numParicles` specifies the number of particles simulated, divided by blocksize (i.e. setting `numParticles` to 50 produces 50*256 particles). `maxIterationsPerFrame` specifies how many steps of the simulation to take before rendering the next frame. For default values for all of these parameters, refer to `sim_param.cpp`.

## Graphics Pipeline

### Rendering
Render targets for all passes except the last use dimensions a bit larger than the window, to prevent popping. This is used when some effects affect neighboring pixels (bloom, ssao..) and must be taken into account even when off-screen.
#### HDR
Each particle is rendered as a fixed-size flare, generated from a gaussian. Particle color depends on velocity, blue at low speeds and purple at high speeds. Additive blending is set, so dense regions look bright. The render target is RGBA16F, because GL_R11F_G11F_B10F looks yellow on subsequent render passes.

#### Bloom

Bloom is applied through a seperable Gaussian blur, applied once in the horizontal and then the vertical direction. The 1D Gaussian kernel is computed by `RendererGL::gaussKernel` and optimized to minimize texel lookups by `RendererGL::optimGaussKernel` following [this guide](https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/). At present, a gaussian window of 49 pixels with sigma = 10.0 is used. Multiple passes are possible (ping pong between two RGBA16F frame buffers), though at present we execute only one blur in each direction.

Note that unlike typical bloom processing, there is no extraction of bright light sources prior to blurring, because the scene (bright stars on a dark background) makes this obsolete.

#### Average luminance
The average luminance of the scene is computed from the HDR target into a downscaled R16F target. Then we generate mipmaps to obtain the average luminance on the smallest mipmap (1x1). (Could also be obtained from a 2x2 texture but screen-size targets always seem to resolve down to odd dimensions)

#### Tonemapping & gamma correction
The exposure of the final render is obtained from the average luminance, and the HDR and Bloom targets are combined and converted to LDR. Gamma correction is also applied. Tada.

## Running headless

If you `nbodygl` on a remote machine with X-forwarding, sending the rendered frames across the net will be a significant bottleneck. This can be worked around by making use of [Xvfb](https://linux.die.net/man/1/xvfb) which provides a *virtual* X display. You can then read from the memory mapped file to write to e.g. MP4 output. 

The script `./scripts/xvfb.sh` runs `nbodygl` in this manner, producing a video file `output.mp4`. Note that this script will run the simulation until manually terminated.

## SYCL vs. CUDA performance

This actually runs faster on SYCL after `dpct` than it does on CUDA. For 4 steps of the physical simulation (1 rendered frame), CUDA takes ~8.14ms whereas SYCL takes ~5.51ms (RTX 3060).
