#!/bin/bash

# Copyright (C) 2022 Codeplay Software Limited
# This work is licensed under the terms of the MIT license.
# For a copy, see https://opensource.org/licenses/MIT.

BUILD_DIR="build_dpcpp"
render=on

if [ -n "$1" ]; then
	if [ "$1" = "no_render" ]; then
		render=off
	else
		echo "Unknown param $1"
		exit
	fi
fi

rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR || exit

#CXX=clang++ \
#CC=clang \
CXX=icpx \
CC=icpx \
cmake ../ \
-DRENDER=${render} \
-DGLEW_LIBRARY=/usr/lib/x86_64-linux-gnu/libGLEW.so \
-DBACKEND=DPCPP -DDPCPP_CUDA_SUPPORT=on || exit

make release
