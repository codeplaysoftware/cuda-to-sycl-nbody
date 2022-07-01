# Copyright (C) 2016 - 2018 Sarah Le Luron
# Copyright (C) 2022 Codeplay Software Limited

find_package(PkgConfig REQUIRED)

pkg_check_modules(Glew REQUIRED IMPORTED_TARGET glew)

find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

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

target_compile_definitions(${BINARY_NAME} PRIVATE -DUSE_OPENGL)

target_compile_features(${BINARY_NAME} PRIVATE cxx_auto_type cxx_nullptr cxx_range_for)

target_include_directories(${BINARY_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${BINARY_NAME} PRIVATE glm::glm glfw PkgConfig::Glew OpenGL::OpenGL)

if(BACKEND STREQUAL "COMPUTECPP")

	find_package(ComputeCpp REQUIRED)

	set(CMAKE_CXX_COMPILER ${ComputeCpp_DEVICE_COMPILER_EXECUTABLE})

	set_property(TARGET ${BINARY_NAME} APPEND
        PROPERTY COMPILE_OPTIONS
		-std=c++17 -sycl-std=2020)

	set_property(SOURCE simulator.dp.cpp APPEND
        PROPERTY COMPILE_OPTIONS
		-O3 -march=native)

	add_sycl_to_target(TARGET ${BINARY_NAME} SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/simulator.dp.cpp)

else() #DPCPP

	if(NOT ((CMAKE_CXX_COMPILER MATCHES ".*dpcpp(\\.exe)?$") OR
                (CMAKE_CXX_COMPILER MATCHES ".*clang\\+\\+(\\.exe)?$")))
		message( FATAL_ERROR "Invalid C++ compiler for SYCL compilation" )
	endif()

	option(DPCPP_CUDA_SUPPORT "on")
	if(DPCPP_CUDA_SUPPORT)
		set(SYCL_FLAGS -fsycl -fsycl-targets=spir64,nvptx64-nvidia-cuda -fsycl-unnamed-lambda)
	else()
		set(SYCL_FLAGS -fsycl -fsycl-targets=spir64 -fsycl-unnamed-lambda)
	endif()

	target_compile_options(${BINARY_NAME} PRIVATE ${SYCL_FLAGS})
	target_link_options(${BINARY_NAME} PRIVATE ${SYCL_FLAGS})

endif()
