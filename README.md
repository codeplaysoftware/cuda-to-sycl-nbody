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

## Passing data between OpenGL & CUDA/SYCL

OpenGL & CUDA are capable of interoperating to share device memory, but this will not play well with the DPC++ Compatibility Tool. Instead, computed particle positions are migrated back to the host by CUDA/SYCL, then sent *back* to OpenGL via mapping.

## Pipeline

### Simulation
The `DiskGalaxySimulator` class handles the physics of n-body interaction. The computation of interparticle forces, velocity & updated particle positions are handled by the CUDA kernel `particle_interaction`.

The equation solved by this code is equivalent to Eq. 1 [here](http://www.scholarpedia.org/article/N-body_simulations_(gravitational)), with the simplifying assumption that all particles have unit mass and there is no external/background force.

---

<img src="https://render.githubusercontent.com/render/math?math=\vec{F}_i=-\sum_{i\neq j} G \frac{(\vec{r}_i - \vec{r}_j)}{{\lvert \vec{r}_i - \vec{r}_j \rvert }^3}">
**TODO**

Add equations here & link force to velocity, velocity to particle position. Describe timestepping & damping.

---

### Rendering
Render targets for all passes except the last use dimensions a bit larger than the window, to prevent popping. This is used when some effects affect neighboring pixels (bloom, ssao..) and must be taken into account even when off-screen.
#### HDR
Each particle is rendered as a fixed-size flare, generated from a gaussian. Particle color depends on velocity, blue at low speeds and purple at high speeds. Additive blending is set, so dense regions look bright. The render target is RGBA16F, because GL_R11F_G11F_B10F looks yellow on subsequent render passes.

#### Bloom
No highpass step since I think all stars should 'shine': no bloom around a flare looks dull. Two RGBA16F half-resolution render targets are used for multiple gaussian blur passes (ping pong targets). It is only a 5-tap blur but it's better for locality even at the cost of several more passes. Horizontal blur passes first, vertical last, to prevent expensive shader switches.

#### Average luminance
The average luminance of the scene is computed from the HDR target into a downscaled R16F target. Then we generate mipmaps to obtain the average luminance on the smallest mipmap (1x1). (Could also be obtained from a 2x2 texture but screen-size targets always seem to resolve down to odd dimensions)

#### Tonemapping & gamma correction
The exposure of the final render is obtained from the average luminance, and the HDR and Bloom targets are combined and converted to LDR. Gamma correction is also applied. Tada.

## Running headless

If you `nbodygl` on a remote machine with X-forwarding, sending the rendered frames across the net will be a significant bottleneck. This can be worked around by making use of [Xvfb](https://linux.die.net/man/1/xvfb) which provides a *virtual* X display. You can then read from the memory mapped file to write to e.g. MP4 output. 

The script `./scripts/xvfb.sh` runs `nbodygl` in this manner, producing a video file `output.mp4`. Note that this script will run the simulation until manually terminated.