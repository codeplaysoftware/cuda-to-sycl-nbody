project (nbody)

add_executable(nbody nbody.cpp camera.cpp sim_param.cpp gen.cpp gl.cpp shader.cpp)

target_compile_features(nbody PRIVATE cxx_auto_type cxx_nullptr cxx_range_for)

find_package(GLM REQUIRED)
find_package(GLFW REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

target_include_directories(nbody PRIVATE 
	${GLM_INCLUDE_DIRS}
	${GLFW_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR})

target_link_libraries(nbody ${GLFW_LIBRARIES} ${GLEW_LIBRARY} ${OPENGL_gl_LIBRARY})