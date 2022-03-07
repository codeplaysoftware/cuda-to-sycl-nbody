#!/bin/bash

# Copyright (C) 2022 Codeplay Software Limited
# This work is licensed under the terms of the MIT license.
# For a copy, see https://opensource.org/licenses/MIT.

BUILD_DIR="build_sycl"

rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR || exit

cmake ../ \
	-DGLEW_LIBRARY=/usr/lib/x86_64-linux-gnu/libGLEW.so \
	-DBUILD_SYCL=on \
	-DUSE_COMPUTECPP=on \
	-DComputeCpp_DIR=/home/joetodd/sources/ComputeCpp \
	-DCOMPUTECPP_BITCODE="host-x86_64" \
	-DSYCL_LANGUAGE_VERSION=202002\
	-DOpenCL_LIBRARY=/usr/lib/x86_64-linux-gnu/libOpenCL.so.1 \
	-DOpenCL_INCLUDE_DIR=/opt/intel/oneapi/compiler/2022.0.1/linux/include/sycl || exit

make
