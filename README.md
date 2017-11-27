# nbody
OpenGL 4.5 N-body simulation - with pretty graphics.

![](http://i.imgur.com/drzi33P.jpg)

Started off as a little compute shader exercise to warm me up, went on because I can't finish anything.

[GPU Gems](http://http.developer.nvidia.com/GPUGems3/gpugems3_ch31.html) was of great help

Graphics inspired by [AndreasReiten's qtvknbody](https://github.com/AndreasReiten/qtvknbody/)

## Building
CMake, GLM, GLFW and GLEW are required.

    mkdir -p build
    cd build
    cmake ..
    make

Remember that you need an OpenGL 4.5 ready graphics card. Radeon 7xxx or Geforce GTX 4xx. No OSX. Maybe Intel.

## Pipeline

### Simulation
Two buffers are created to hold the position and velocity data of particles. 'Random' data is fed into these buffers for initialization, in the shape of a thick disk.

There are two steps to compute the N-body simulation:
#### Interaction
This compute shader runs for each particle and computes the gravitational contribution of all other particles. An epsilon value is chosen so particles don't fly away at lightspeed, some damping is applied to grossly approximate interstellar medium. Chunks of particles are loaded into shared memory to make it work like a handcrafted cache and boost the performance. This pass only updates the particles' velocities.
#### Integration
From the new velocities, the particles' positions are computed. That's all. This is done as a separate step because it guarantees some kind of double buffering between positions and velocities.

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



