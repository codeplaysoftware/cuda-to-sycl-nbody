# Copyright (C) 2016 - 2018 Sarah Le Luron
# Copyright (C) 2022 Codeplay Software Limited

cmake_minimum_required (VERSION 3.16)

project (nbody LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(BACKEND "CUDA" CACHE STRING "Which backend to build")

if(BACKEND STREQUAL "CUDA")
	enable_language(CUDA)
	add_subdirectory(src)
elseif(BACKEND STREQUAL "DPCPP" OR BACKEND STREQUAL "COMPUTECPP")
	add_subdirectory(src_sycl)
else()
	message(FATAL_ERROR "Unrecognized BACKEND")
endif()

add_subdirectory(libs/imgui)
