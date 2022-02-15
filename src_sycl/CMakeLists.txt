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

target_compile_features(nbodygl PRIVATE cxx_auto_type cxx_nullptr cxx_range_for)

target_include_directories(nbodygl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
	/opt/intel/oneapi/mkl/2022.0.1/include/
	)

target_link_libraries(nbodygl PRIVATE glm::glm glfw PkgConfig::Glew OpenGL::OpenGL)
