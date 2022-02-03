# Copyright (C) 2016 - 2018 Sarah Le Luron
# Copyright (C) 2022 Codeplay Software Limited

find_package(GLM REQUIRED)
find_package(GLFW REQUIRED)
find_package(GLEW REQUIRED)
set(OpenGL_GL_PREFERENCE GLVND)
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

add_executable(nbodygl ${COMMON_SOURCE} ${OPENGL_SOURCE})

target_compile_definitions(nbodygl PRIVATE -DUSE_OPENGL)

target_compile_features(nbodygl PRIVATE cxx_auto_type cxx_nullptr cxx_range_for)

target_include_directories(nbodygl PRIVATE 
	${GLM_INCLUDE_DIRS}
	${GLFW_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
    ${CUDA_INCLUDE_DIRS})

target_link_libraries(nbodygl PRIVATE ${GLFW_LIBRARIES} ${GLEW_LIBRARY} ${OPENGL_LIBRARY})
