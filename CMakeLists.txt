# Copyright (C) 2016 - 2018 Sarah Le Luron
# Copyright (C) 2022 Codeplay Software Limited

cmake_minimum_required (VERSION 3.16)

project (nbody LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(BACKEND "CUDA" CACHE STRING "Which backend to build")
option(RENDER "Use openGl or not" ON)

if(BACKEND STREQUAL "CUDA")
  set(BINARY_NAME "nbody_cuda" CACHE STRING "Binary name")
  enable_language(CUDA)
  add_subdirectory(src)
elseif(BACKEND STREQUAL "DPCPP")
  set(BINARY_NAME "nbody_dpcpp" CACHE STRING "Binary name")
  add_subdirectory(src_sycl)
else()
  message(FATAL_ERROR "Unrecognized BACKEND")
endif()

if(RENDER)
  add_subdirectory(libs/imgui)
endif()

