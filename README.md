# nbody
Accelerated N-body sim with OpenGL graphics & automatic CUDA->SYCL conversion using [dpct](https://www.intel.com/content/www/us/en/developer/tools/oneapi/dpc-compatibility-tool.html).

![](http://i.imgur.com/drzi33P.jpg)

Forked from https://github.com/salel/nbody

## Compilers/Backends

This nbody simulation can be run with any of:
 - CUDA
 - DPC++ CUDA backend
 - DPC++ OpenCL CPU backend
 - ComputeCpp OpenCL Backend

Source code for the CUDA version is in `./src/` while `./src_sycl/` contains the semi-automatically converted SYCL code.

## Build Dependencies

### Graphics Dependencies

The rendering components of this code are independent of the CUDA/SYCL backend, and depend on:
 - GLM
 - GLFW
 - GLEW

These can be installed with apt:
```
sudo apt update
sudo apt install libglew-dev libglfw3-dev libglm-dev libxxf86vm-dev libxcursor-dev libxinerama-dev libxi-dev
```

The implementation relies on OpenGL 4.5.

### Simulation Dependencies (CUDA & SYCL)

The CUDA version of this code requires the [CUDA runtime](https://intel.github.io/llvm-docs/GetStartedGuide.html#build-dpc-toolchain-with-support-for-nvidia-cuda) to be installed on your machine.

The DPC++ CUDA backend version also requires the CUDA runtime.

The DPC++ OpenCL backend requires an [OpenCL runtime](https://intel.github.io/llvm-docs/GetStartedGuide.html#install-low-level-runtime). To run specifically on the CPU, you must install the OpenCL runtime for your CPU.

Both DPC++ backends require the [DPC++ compiler](https://intel.github.io/llvm-docs/GetStartedGuide.html) to compile the SYCL code.

The ComputeCpp backend requires the new Experimental version of the ComputeCpp compiler, which you can download from the [Codeplay website](https://developer.codeplay.com/products/computecpp/ce/download?experimental=true).


## Building

This project uses CMake for build configuration. Build scripts for CUDA, DPC++ and ComputeCpp are located in `./scripts/`. Note that these scripts include some hardcoded paths from our dev machine, and so will not work out-the-box.

The CMake option `-DBACKEND` allows to select which backend ("CUDA", "DPCPP", or "COMPUTECPP") to build. CUDA is built by default. The name of the built binary is suffixed with the backend (`nbody_cuda`, `nbody_dpcpp`, or `nbody_computecpp`).

The DPC++ backend, in turn, supports both an OpenCL & CUDA backend, both of which are built by default. If you are building on a machine without CUDA support, you can switch off the DPC++ CUDA backend with the flag `-DDPCPP_CUDA_SUPPORT=off`.

## Converting CUDA to SYCL

The script `./scripts/run_dpct.sh` calls a containerized version of the DPC++ Compatibility Tool to automatically convert the CUDA components of this project into SYCL. A docker container was used because the dev machine has an incompatible version of the CUDA driver. This should be adapted based on your environment. 

The DPC++ compatibility tool offers options for intercepting complex builds, but current dev environment restrictions require me to run dpct inside a docker container. This complicates things, so for now I'm just doing single source conversion on the simulator.cu file.

## Running on different platforms

The script `./scripts/run_nbody.sh` will run the nbody simulation, selecting a different binary based on the `-b` flag, where `-b` can be any of: `cuda`, `dpcpp`, `computecpp`. Subsequent positional arguments are passed on to the `nbody` binary. These positions args are described in the [Simulation](#Simulation) section. For example, to run on the DPC++ OpenCL host backend with 25600 (100 * 256) particles, executing 10 timesteps per rendered frame:

```
./scripts/run_nbody.sh -b dpcpp 100 10
```

Note that this script runs `nbody` with the default X window, as opposed to using [xvfb](#Running-headless). This makes it unsuitable for running on a remote machine.

`run_nbody.sh` is a simple wrapper around the `nbody_*` binaries with some environment variables set; the sections below describe how to launch the binaries directly.

### Detecting available SYCL backends

The `sycl-ls` tool allows you to check for available backends on the system. For example, on a system with Intel OpenCL CPU runtime & CUDA runtime, the output is:

```
> sycl-ls
[opencl:cpu:0] Intel(R) OpenCL, Intel(R) Core(TM) i7-6700K CPU @ 4.00GHz 3.0 [2021.13.11.0.23_160000]
[opencl:cpu:1] Intel(R) OpenCL, Intel(R) Core(TM) i7-6700K CPU @ 4.00GHz 3.0 [2021.13.11.0.23_160000]
[cuda:gpu:0] NVIDIA CUDA BACKEND, NVIDIA GeForce RTX 3060 0.0 [CUDA 11.6]
[host:host:0] SYCL host platform, SYCL host device 1.2 [1.2]
```

### Selecting a backend (DPC++)

By specifying the environment variable `SYCL_DEVICE_FILTER`, it's possible to switch between running with the CUDA backend and the OpenCL host backend. For example:

```
    SYCL_DEVICE_FILTER=cuda ./nbody_dpcpp
```
will run on the CUDA backend, whereas:
```
    SYCL_DEVICE_FILTER=opencl:cpu ./nbody_dpcpp
```
will run on a CPU through the OpenCL backend. Note the correspondence between options for `SYCL_DEVICE_FILTER` and the output of `sycl-ls`.

**Note**: Selection between DPC++ backends at runtime is possible because `CMakeLists.txt` specifies building the SYCL code for both CUDA (`nvptx64-nvidia-cuda`) & OpenCL (`spir64`) targets:
```
     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsycl -fsycl-targets=spir64,nvptx64-nvidia-cuda -fsycl-unnamed-lambda")
```

### Selecting the ComputeCpp Backend

By specifying the environment variable `COMPUTECPP_TARGET`, it's possible to switch between running with the OpenCL CPU, OpenCL GPU or the host backends:

```
COMPUTECPP_TARGET=host ./nbody_computecpp
COMPUTECPP_TARGET=cpu ./nbody_computecpp
COMPUTECPP_TARGET=gpu ./nbody_computecpp
```

Note that the ComputeCpp version will only support backends with USM support (Intel GPUs/CPUs).

### Adapting the project for DPC++ OpenCL

No changes to the code were required, but there were a couple of bugs which are worked around.

Firstly, when building for multiple targets (`-fsycl-targets`), there is a [recent bug](https://github.com/intel/llvm/issues/5330) which causes failure to link to static libraries. The workaround for this is to switch from building `imgui` as a static to a shared library.

Secondly, I encountered the common CL header bug (see [here](https://github.com/intel/llvm/issues/2617) and [here](https://github.com/oneapi-src/oneDNN/issues/885)). This turned out to be triggered for the `spir64` backend because the CUDA headers were included *only* via `-I` and not via `-internal-isystem`. This caused them to take precedence over SYCL CL headers. The solution was to not include CUDA headers in `src_sycl/CMakeLists.txt`, which turned out to be unnecessary anyway.

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

`./nbody_cuda numParticles simIterationsPerFrame damping dt distEps G`

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

The appearance & performance of the blur is controlled by four variables which are not currently exposed as arguments to `nbody_[backend]` but which could be manually modified as desired. The two arguments to the `gaussKernel` function (`sigma` and `halfwidth`) effectively define the 'spread' of the blur. Higher values for `sigma` result in wider blurring, whereas `halfwindow` defines the actual width of the pixel window which is sampled. Higher values of `halfwindow` will decrease performance, as more texel lookups are required. As a general rule, when increasing `sigma`, it will likely be necessary to increase `halfwindow` to avoid an obvious visual cut-off at the edge of the window. Conversely, a wide `halfwindow` with a small `sigma` reduces performance unnecessarily, because texels with negligible contribution will be sampled.

Blur downscaling is a common technique to improve blur performance; the image is downsampled by the factor `blur_dsc` defined in `renderer_gl.cpp`, then the regular blur filter is performed, and finally the image is upscaled again. This is a very cheap way of enhancing the blur effect, but there is an associated artefact:

![DownscaleArtefact](/docs/downscale_artefact.png)

If this artefact is unacceptable, set `blur_dsc = 1` to turn off downscaling. Note however that this will significantly reduce the blurriness, and compensating with wider `halfwindow` or more passes (see below) will cost a lot of rendering time.

Enhanced blurring can also be achieved by executing multiple passes. This is controlled by `nPasses`, and is set to 1 by default. Due to the dominance of blur in the render pipeline, total rendering time should scale pretty much linearly with `nPasses`, so increasing it is a potentially expensive option.

#### Average luminance
The average luminance of the scene is computed from the HDR target into a downscaled R16F target. Then we generate mipmaps to obtain the average luminance on the smallest mipmap (1x1). (Could also be obtained from a 2x2 texture but screen-size targets always seem to resolve down to odd dimensions)

#### Tonemapping & gamma correction
The exposure of the final render is obtained from the average luminance, and the HDR and Bloom targets are combined and converted to LDR. Gamma correction is also applied. Tada.

## Running headless

If you run `nbody_cuda` on a remote machine with X-forwarding, sending the rendered frames across the net will be a significant bottleneck. This can be worked around by making use of [Xvfb](https://linux.die.net/man/1/xvfb) which provides a *virtual* X display. You can then read from the memory mapped file to write to e.g. MP4 output. 

The script `./scripts/xvfb.sh` runs `nbody_cuda` in this manner, producing a video file `output.mp4`. Note that this script will run the simulation until manually terminated.

## Performance Scaling for Demos

We've previously discussed the desire for a simulation which is *visibly* slower when the physics kernel isn't well optimized. With current default settings, the rendering takes longer (~55ms) than the simulation (10ms). However, altering three of the simulation parameters provides almost complete control of the ratio of render to simulation time.

Firstly, the number of particles (`numParticles` [above](#Simulation)) has a large effect on the simulation time, as the computation scales with O(n<sup>2</sup>). By default, 12.8k particles (50 * 256) are rendered, but increasing this to 64k particles (250 * 256), the simulation time increases from 10ms to ~170ms.

Alternatively, simulation time can be arbitrarily raised or lowered by changing both timestep size (`dt` [above](#Simulation)) and simultion steps per rendered frame (`simIterationsPerFrame`, [above](#Simulation)). By default, a timestep size of 0.005 is used, and 4 simulation steps are taken per rendered frame (Note that `scripts/xvfb.sh` overrides these default values with `dt = 0.001` and `simIterationsPerFrame = 5`).

To increase the simulation time by a factor of 5, for example, simply divide `dt` by 5 and multiply `simIterationsPerFrame` by 5. This will produce *almost* identical output. Take care with *increasing* `dt` to get the opposite effect; above a certain value, the simulation will become unstable & you may see this manifest as unphysical behaviour (very fast moving stars exploding out from the centre). Instability at large `dt` can be mitigated, to an extent, by increasing `distEps` or `damping`.

A significant portion of the rendering time is the bloom filter. The [bloom](#Bloom) section has some tips about how to control this.

## SYCL vs. CUDA performance

This repo previously reported *faster* performance from SYCL than CUDA, but this was due to an erroneous translation in DPCT from `__frsqrt_rn` to `sycl::rsqrt`. The former has higher precision and runs slower than the latter. This has now been rectified so that the original CUDA code calls `rsqrt`.

With this bug rectified, and without any further modification to the CUDA code or dpct-generated SYCL code, the SYCL code is considerably slower because DPCT inserts a cast to double in the rsqrt call:

```
         coords_t inv_dist_cube =
             sycl::rsqrt((double)dist_sqr * dist_sqr * dist_sqr);

```

This is presumably because DPCT is unsure of the equivalence of `rsqrt` and `sycl::rsqrt`. However, inspecting PTX reveals that the generated instructions are the same, so the cast to double is unnecessary. Removing the cast to double leaves a 40% performance gap between CUDA and SYCL.

The root cause of this 40% performance gap appears to be different handling of the branch instruction:

```
if (i == id) continue;
```
in the main loop in simulation.dp.cpp. Whereas NVCC handles this via instruction predication, DPC++ generates branch & sync instructions. By replacing this branch instruction with an arithemtic:

```
force += r * inv_dist_cube * (i != id);
```
in both the CUDA & SYCL code, we get the same performance between the two. For 5 steps of the physical simulation (1 rendered frame) with 12,800 particles, both CUDA and SYCL take ~5.05ms (RTX 3060).
