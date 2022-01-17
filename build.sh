#!/bin/bash

rm -rf build/
mkdir build
cd build

cmake ../ -DGLEW_LIBRARY=/usr/lib/x86_64-linux-gnu/libGLEW.so -DCMAKE_EXPORT_COMPILE_COMMANDS=on
make
