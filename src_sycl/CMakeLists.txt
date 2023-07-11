# Copyright (C) 2016 - 2018 Sarah Le Luron
# Copyright (C) 2022 Codeplay Software Limited

find_package(PkgConfig REQUIRED)

pkg_check_modules(Glew REQUIRED IMPORTED_TARGET glew)

find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(dpct REQUIRED)

set(COMMON_SOURCE 
	nbody.cpp 
	camera.cpp 
	sim_param.cpp 
	gen.cpp 
	simulator.dp.cpp)

set(OPENGL_SOURCE 
	renderer_gl.cpp 
	shader.cpp)

add_executable(${BINARY_NAME} ${COMMON_SOURCE} ${OPENGL_SOURCE})

target_compile_definitions(${BINARY_NAME} PRIVATE -DUSE_OPENGL COMPILER_NAME="SYCL")

target_compile_features(${BINARY_NAME} PRIVATE cxx_auto_type cxx_nullptr cxx_range_for)

target_include_directories(${BINARY_NAME} PRIVATE ${dpct_INCLUDE_DIR})

target_link_libraries(${BINARY_NAME} PRIVATE glm::glm glfw PkgConfig::Glew OpenGL::OpenGL)

if(BACKEND STREQUAL "COMPUTECPP")

	find_package(ComputeCpp REQUIRED)

	set(CMAKE_CXX_COMPILER ${ComputeCpp_DEVICE_COMPILER_EXECUTABLE})

	set_property(TARGET ${BINARY_NAME} APPEND
		PROPERTY COMPILE_OPTIONS
		-sycl-std=2020)

	set_target_properties(${BINARY_NAME} 
		PROPERTIES CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF)

	set_property(SOURCE simulator.dp.cpp APPEND
        PROPERTY COMPILE_OPTIONS
		-O3 -march=native)

	add_sycl_to_target(TARGET ${BINARY_NAME} SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/simulator.dp.cpp)

else() #DPCPP

	if(NOT ((CMAKE_CXX_COMPILER MATCHES ".*dpcpp(\\.exe)?$") OR
                (CMAKE_CXX_COMPILER MATCHES ".*icpx(\\.exe)?$")  OR
                (CMAKE_CXX_COMPILER MATCHES ".*clang\\+\\+(\\.exe)?$")))
		message( FATAL_ERROR "Invalid C++ compiler for SYCL compilation" )
	endif()

	option(DPCPP_CUDA_SUPPORT "on")
	if(DPCPP_CUDA_SUPPORT)
		set(DEFAULT_CUDA_COMPUTE_CAPABILITY "86")
		execute_process(
			COMMAND bash -c "which nvidia-smi >/dev/null && nvidia-smi --query-gpu=compute_cap --format=csv,noheader | head -n 1 | tr -d '.'"
			OUTPUT_VARIABLE CUDA_COMPUTE_CAPABILITY
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		if ("${CUDA_COMPUTE_CAPABILITY}" STREQUAL "")
			set(CUDA_COMPUTE_CAPABILITY ${DEFAULT_CUDA_COMPUTE_CAPABILITY})
		endif()
		set(SYCL_FLAGS -fsycl
                  -fsycl-targets=spir64,nvptx64-nvidia-cuda
                  -Xsycl-target-backend=nvptx64-nvidia-cuda --cuda-gpu-arch=sm_${CUDA_COMPUTE_CAPABILITY}
                  -fsycl-unnamed-lambda
                  -fgpu-inline-threshold=100000
                  -Ofast)
	else()
		set(SYCL_FLAGS -fsycl -fsycl-targets=spir64 -fsycl-unnamed-lambda)
	endif()

	target_compile_options(${BINARY_NAME} PRIVATE ${SYCL_FLAGS})
	target_link_options(${BINARY_NAME} PRIVATE ${SYCL_FLAGS})

endif()
