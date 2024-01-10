# Copyright (C) 2016 - 2018 Sarah Le Luron
# Copyright (C) 2022 Codeplay Software Limited

find_package(PkgConfig REQUIRED)

if (RENDER) 
  pkg_check_modules(Glew REQUIRED IMPORTED_TARGET glew)

  find_package(glm REQUIRED)
  find_package(glfw3 REQUIRED)
  find_package(OpenGL REQUIRED)
endif()

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

set(DEBUG_FLAGS -g -O0)


if (RENDER) 
  set(RENDER_LIB  glm::glm glfw PkgConfig::Glew OpenGL::OpenGL cuda)
  set(RENDER_FLAG -DUSE_OPENGL)
  set(SOURCE_FILES ${COMMON_SOURCE} ${OPENGL_SOURCE})
else()
  set(RENDER_LIB  glm::glm cuda)
  set(RENDER_FLAG DISABLE_GL)
  set(SOURCE_FILES ${COMMON_SOURCE})
endif()

add_custom_target(release DEPENDS ${BINARY_NAME})
add_executable(${BINARY_NAME} ${SOURCE_FILES})
# COMPILER_NAME here is only used to print text overlay on simulation
target_compile_definitions(${BINARY_NAME} PRIVATE ${RENDER_FLAG} COMPILER_NAME="CUDA")
target_link_libraries(${BINARY_NAME} PRIVATE ${RENDER_LIB})
target_compile_features(${BINARY_NAME} PRIVATE cxx_auto_type cxx_nullptr cxx_range_for)
target_include_directories(${BINARY_NAME} PRIVATE ${CUDA_INCLUDE_DIRS})

add_custom_target(debug DEPENDS ${BINARY_NAME}_d)
add_executable(${BINARY_NAME}_d ${SOURCE_FILES})
# COMPILER_NAME here is only used to print text overlay on simulation
target_compile_definitions(${BINARY_NAME}_d PRIVATE ${RENDER_FLAG} COMPILER_NAME="CUDA")
target_link_libraries(${BINARY_NAME}_d PRIVATE ${RENDER_LIB})
target_compile_features(${BINARY_NAME}_d PRIVATE cxx_auto_type cxx_nullptr cxx_range_for)
target_include_directories(${BINARY_NAME}_d PRIVATE ${CUDA_INCLUDE_DIRS})
target_compile_options(${BINARY_NAME}_d PRIVATE ${DEBUG_FLAGS})
