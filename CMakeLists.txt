# Copyright (C) 2016 - 2018 Sarah Le Luron
# Copyright (C) 2022 Codeplay Software Limited

cmake_minimum_required (VERSION 3.16)

project (nbody LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(BACKEND "CUDA" CACHE STRING "Which backend to build")

option(PRINT_KERNEL_TIME "Whether to overlay kernel time on display" ON)
if(PRINT_KERNEL_TIME)
  set(BINARY_NAME "" CACHE STRING "Binary name")
  add_compile_definitions(PRINT_FPS)
else()
  set(BINARY_NAME "_no_kernel_time" CACHE STRING "Binary name")
endif()

if(BACKEND STREQUAL "CUDA")
        string(PREPEND BINARY_NAME "nbody_cuda")
	enable_language(CUDA)
	add_subdirectory(src)
elseif(BACKEND STREQUAL "DPCPP")
        string(PREPEND BINARY_NAME "nbody_dpcpp")
	add_subdirectory(src_sycl)
elseif(BACKEND STREQUAL "COMPUTECPP")
        string(PREPEND BINARY_NAME "nbody_computecpp")
	add_subdirectory(src_sycl)
else()
	message(FATAL_ERROR "Unrecognized BACKEND")
endif()

add_subdirectory(libs/imgui)
