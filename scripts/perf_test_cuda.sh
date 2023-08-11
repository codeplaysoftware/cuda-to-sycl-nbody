#!/bin/bash

# Copyright (C) 2022 Codeplay Software Limited
# This work is licensed under the terms of the MIT license.
# For a copy, see https://opensource.org/licenses/MIT.

# Get rid of any previous virtual frame buffer
pkill -9 Xvfb
rm /var/tmp/Xvfb_screen_0

# Create a virtual screen :99.0 with given dimensions & color depth
# mapping output to /var/tmp/Xvfb_screen_0
Xvfb :99 -screen 0 1920x1080x16 -fbdir /var/tmp &

#DISPLAY=:99.0 SYCL_DEVICE_FILTER=opencl:cpu ./nbody_dpcpp 50 5 0.999 0.001 1.0e-3 2.0 &
#DISPLAY=:99.0 SYCL_DEVICE_FILTER=cuda ./nbody_dpcpp 50 5 0.999 0.001 1.0e-3 2.0 &
DISPLAY=:99.0 ./nbody_cuda 50 5 0.999 0.001 1.0e-3 2.0
