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

add_executable(nbodygl ${COMMON_SOURCE} ${OPENGL_SOURCE})

target_compile_definitions(nbodygl PRIVATE -DUSE_OPENGL)

target_compile_features(nbodygl PRIVATE cxx_auto_type cxx_nullptr cxx_range_for cxx_std_17)

target_include_directories(nbodygl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(nbodygl PRIVATE glm::glm glfw PkgConfig::Glew OpenGL::OpenGL)

option(USE_COMPUTECPP "whether to use ComputeCpp compiler" OFF)

if(USE_COMPUTECPP)
	find_package(ComputeCpp REQUIRED)

	set(CMAKE_CXX_COMPILER ${ComputeCpp_DEVICE_COMPILER_EXECUTABLE})

	set_property(SOURCE simulator.dp.cpp APPEND
        PROPERTY COMPILE_OPTIONS
		-O3 -march=native)

	add_sycl_to_target(TARGET nbodygl SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/simulator.dp.cpp)
else()

	if(NOT ((CMAKE_CXX_COMPILER MATCHES ".*dpcpp(\\.exe)?$") OR
                (CMAKE_CXX_COMPILER MATCHES ".*clang\\+\\+(\\.exe)?$")))
		message( FATAL_ERROR "Invalid C++ compiler for SYCL compilation" )
	endif()

	set(SYCL_FLAGS -fsycl -fsycl-targets=spir64,nvptx64-nvidia-cuda -fsycl-unnamed-lambda)
	target_compile_options(nbodygl PRIVATE ${SYCL_FLAGS})
	target_link_options(nbodygl PRIVATE ${SYCL_FLAGS})
endif()
