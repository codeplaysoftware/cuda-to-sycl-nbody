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

if(NOT TARGET glm::glm)
  add_library(glm::glm IMPORTED INTERFACE)
  target_include_directories(glm::glm INTERFACE ${GLM_INCLUDE_DIR})
endif()

add_executable(${BINARY_NAME} ${COMMON_SOURCE} ${OPENGL_SOURCE})

# COMPILER_NAME here is only used to print text overlay on simulation
target_compile_definitions(${BINARY_NAME} PRIVATE -DUSE_OPENGL COMPILER_NAME="CUDA")

target_compile_features(${BINARY_NAME} PRIVATE cxx_auto_type cxx_nullptr cxx_range_for)

target_include_directories(${BINARY_NAME} PRIVATE ${CUDA_INCLUDE_DIRS})

target_link_libraries(${BINARY_NAME} PRIVATE glm::glm glfw PkgConfig::Glew OpenGL::OpenGL cuda)
