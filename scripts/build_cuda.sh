#!/bin/bash

# Copyright (C) 2022 Codeplay Software Limited
# This work is licensed under the terms of the MIT license.
# For a copy, see https://opensource.org/licenses/MIT.

BUILD_DIR="build_cuda"
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

cmake ../ \
-DRENDER=${render} \
-DGLEW_LIBRARY=/usr/lib/x86_64-linux-gnu/libGLEW.so \
-DCMAKE_EXPORT_COMPILE_COMMANDS=on || exit

make
