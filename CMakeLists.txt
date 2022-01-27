cmake_minimum_required (VERSION 3.16)

project (nbody LANGUAGES CXX CUDA)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(BUILD_SYCL OFF CACHE STRING "")
if(BUILD_SYCL)
        add_subdirectory(src_sycl)
else()
        add_subdirectory(src)
endif()

