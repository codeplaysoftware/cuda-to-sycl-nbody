# Copyright (C) 2016 - 2018 Sarah Le Luron
# Copyright (C) 2022 Codeplay Software Limited

find_package(PkgConfig REQUIRED)

pkg_check_modules(Glew REQUIRED IMPORTED_TARGET glew)

find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

find_package(CUDA REQUIRED)

set(COMMON_SOURCE 
	nbody.cpp 
	camera.cpp 
	sim_param.cpp 
	gen.cpp 
	simulator.cu)

set(OPENGL_SOURCE 
	renderer_gl.cpp 
	shader.cpp)

add_executable(${BINARY_NAME} ${COMMON_SOURCE} ${OPENGL_SOURCE})

target_compile_definitions(${BINARY_NAME} PRIVATE -DUSE_OPENGL)

target_compile_features(${BINARY_NAME} PRIVATE cxx_auto_type cxx_nullptr cxx_range_for)

target_include_directories(${BINARY_NAME} PRIVATE ${CUDA_INCLUDE_DIRS})

target_link_libraries(${BINARY_NAME} PRIVATE glm::glm glfw PkgConfig::Glew OpenGL::OpenGL)
