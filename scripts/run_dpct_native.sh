#!/bin/bash

# Copyright (C) 2022 Codeplay Software Limited
# This work is licensed under the terms of the MIT license.
# For a copy, see https://opensource.org/licenses/MIT.

# This script converts the project's CUDA code to SYCL code. The DPC++ compatibility tool offers options
# for intercepting complex builds, but current dev environment restrictions require me to run dpct inside
# a docker container. This complicates things, so for now I'm just doing single source conversion on the 
# simulator.cu file.
#
# The option --assume-nd-range-dim=1 prevents dpct from converting CUDA 1D ranges into SYCL 3D ranges.
# It's not totally clear why the default behaviour isn't just to keep the CUDA dimensionality.
#
# The custom helper header files referred to by the --use-custom-helper flag are already part of this repo.
# As such, we suppress generation of new helper
# headers when calling dpct with `--use-custom-helper=none`.

export NBODY_DIR=$PWD

cd $NBODY_DIR

rm src_sycl/*.[ch]pp src_sycl/*.yaml
cd src_sycl; ln -s ../src/*[ch]pp .; cd -

dpct --out-root=./src_sycl \
    --assume-nd-range-dim=1 \
    --use-custom-helper=none \
    --stop-on-parse-err \
    --sycl-named-lambda \
    ./src/simulator.cu

sed -i 's/simulator.cuh/simulator.dp.hpp/g' src_sycl/renderer.hpp
sed -i 's/simulator.cuh/simulator.dp.hpp/g' src_sycl/nbody.cpp

# -p=/nbody/build \
# --optimize-migration
