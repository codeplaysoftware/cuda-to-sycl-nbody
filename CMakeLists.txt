# Copyright (C) 2016 - 2018 Sarah Le Luron
# Copyright (C) 2022 Codeplay Software Limited

cmake_minimum_required (VERSION 3.16)

project (nbody LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

option(BUILD_SYCL "whether to build SYCL" OFF)

if(BUILD_SYCL)
        add_subdirectory(src_sycl)
else()
	enable_language(CUDA)
        add_subdirectory(src)
endif()

add_subdirectory(libs/imgui)
