# nbody
CUDA N-body sim with OpenGL graphics & automatically CUDA->SYCL conversion using [dpct](https://www.intel.com/content/www/us/en/developer/tools/oneapi/dpc-compatibility-tool.html).

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

`./nbodygl numParticles simIterationsPerFrame damping dt distEps G`

Note that `numParticles` specifies the number of particles simulated, divided by blocksize (i.e. setting `numParticles` to 50 produces 50*256 particles). `simIterationsPerFrame` specifies how many steps of the simulation to take before rendering the next frame. For default values for all of these parameters, refer to `sim_param.cpp`.

### Modifying Simulation Behaviour

You can get quite a wide range of 'galactic' behaviours by playing with the parameters described above.

Initial velocity of stars is a stable orbital velocity, computed with an implicit value for gravity of `G = 1`. The default value *during* the simulation, however, is `G = 2`. So by default the galaxy collapses inwards quite quickly, but by reducing G closer to 1, you can make a more stable, rotating galaxy.

The `damping` factor is a drag term. By default `damping = 0.999998` but by reducing this value to e.g. `0.999`, stars will tend to form local clusters before collapsing in towards the galactic centre.

`distEps` serves as a stabilising parameter to prevent numerical instability at larger timestep sizes. Setting this value very small (`1.0e-10`) will produce more 'explosive' simulations. This is unrealistic for n-body gravitational interaction, but it looks dramatic.

If you want to speed up the evolution of the galaxy, set a larger timestep size (`dt`) or increase the number of steps taken per frame (`simIterationsPerFrame`). Either change will increase the total simulation time per rendered frame. If you reach a sufficiently high timestep size that you get unstable explosive behaviour, increase the value of `distEps` and this should stabilise things. Note that there is a separate discussion [below](#performance-scaling-for-demos) about altering the ratio of compute/render time to, for instance, visually highlight a performance difference between platforms.

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

## Performance Scaling for Demos

We've previously discussed the desire for a simulation which is *visibly* slower when the physics kernel isn't well optimized. With current default settings, the rendering takes longer (~55ms) than the simulation (10ms). However, altering three of the simulation parameters provides almost complete control of the ratio of render to simulation time.

Firstly, the number of particles (`numParticles` [above](#Simulation)) has a large effect on the simulation time, as the computation scales with O(n<sup>2</sup>). By default, 12.8k particles (50 * 256) are rendered, but increasing this to 64k particles (250 * 256), the simulation time increases from 10ms to ~170ms.

Alternatively, simulation time can be arbitrarily raised or lowered by changing both timestep size (`dt` [above](#Simulation)) and simultion steps per rendered frame (`simIterationsPerFrame`, [above](#Simulation)). By default, a timestep size of 0.005 is used, and 4 simulation steps are taken per rendered frame (Note that `scripts/xvfb.sh` overrides these default values with `dt = 0.001` and `simIterationsPerFrame = 5`).

To increase the simulation time by a factor of 5, for example, simply divide `dt` by 5 and multiply `simIterationsPerFrame` by 5. This will produce *almost* identical output. Take care with *increasing* `dt` to get the opposite effect; above a certain value, the simulation will become unstable & you may see this manifest as unphysical behaviour (very fast moving stars exploding out from the centre). Instability at large `dt` can be mitigated, to an extent, by increasing `distEps` or `damping`.

## SYCL vs. CUDA performance

This actually runs faster on SYCL after `dpct` than it does on CUDA. For 4 steps of the physical simulation (1 rendered frame), CUDA takes ~8.14ms whereas SYCL takes ~5.51ms (RTX 3060).
