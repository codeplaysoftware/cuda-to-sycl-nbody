#!/bin/bash

BUILD_DIR="build_cuda"

rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR

cmake ../ -DGLEW_LIBRARY=/usr/lib/x86_64-linux-gnu/libGLEW.so -DCMAKE_EXPORT_COMPILE_COMMANDS=on
make
