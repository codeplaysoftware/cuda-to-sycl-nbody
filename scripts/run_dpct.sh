#!/bin/bash

# This script converts the project's CUDA code to SYCL code. The DPC++ compatibility tool offers options
# for intercepting complex builds, but current dev environment restrictions require me to run dpct inside
# a docker container. This complicates things, so for now I'm just doing single source conversion on the 
# simulator.cu file.
#
# The option --assume-nd-range-dim=1 prevents dpct from converting CUDA 1D ranges into SYCL 3D ranges.
# It's not totally clear why the default behaviour isn't just to keep the CUDA dimensionality.

rm src_sycl/*.[ch]pp src_sycl/*.yaml
cp -r src/*[ch]pp src_sycl/
#cp -r src/*cpp src_sycl/

docker run --rm \
    -v /opt/intel/oneapi/dpcpp-ct/2022.0.0/:/dpcpp-ct \
    -v $PWD:/nbody/ \
    -u $UID \
    -it joeatodd/onednn-cuda \
    /dpcpp-ct/bin/dpct --out-root=/nbody/src_sycl \
    --assume-nd-range-dim=1 \
    --use-custom-helper=all \
    --stop-on-parse-err \
    /nbody/src/simulator.cu

sed -i 's/simulator.cuh/simulator.dp.hpp/g' src_sycl/renderer.hpp
sed -i 's/simulator.cuh/simulator.dp.hpp/g' src_sycl/nbody.cpp

# -p=/nbody/build \
# --optimize-migration
