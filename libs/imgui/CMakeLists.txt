add_library(imgui STATIC
    src/imgui.cpp
    src/imgui_widgets.cpp
    src/imgui_demo.cpp
    src/imgui_draw.cpp
    src/imgui_tables.cpp
    src/imgui_impl_opengl3.cpp
    src/imgui_impl_glfw.cpp)

target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_include_directories(nbodygl PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

# Link main project to imgui lib
target_link_libraries(nbodygl PRIVATE imgui)
