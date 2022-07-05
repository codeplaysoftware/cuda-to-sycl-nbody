#!/bin/bash

# Copyright (C) 2022 Codeplay Software Limited
# This work is licensed under the terms of the MIT license.
# For a copy, see https://opensource.org/licenses/MIT.

BUILD_DIR="build_computecpp"

rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR || exit

cmake ../ \
	-DGLEW_LIBRARY=/usr/lib/x86_64-linux-gnu/libGLEW.so \
	-DBACKEND="COMPUTECPP" \
	-DComputeCpp_DIR=$HOME/sources/ComputeCpp \
	-DSYCL_LANGUAGE_VERSION=202003 || exit

make
