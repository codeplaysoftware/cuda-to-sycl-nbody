#!/bin/bash

BUILD_DIR="build_sycl"

rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR

CXX=clang++ \
CC=clang \
CXXFLAGS="-fsycl -fsycl-targets=nvptx64-nvidia-cuda -fsycl-unnamed-lambda" \
cmake ../ \
-DGLEW_LIBRARY=/usr/lib/x86_64-linux-gnu/libGLEW.so \
-DBUILD_SYCL=on

make
