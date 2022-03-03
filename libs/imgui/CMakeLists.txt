add_library(imgui SHARED
    src/imgui.cpp
    src/imgui_widgets.cpp
    src/imgui_demo.cpp
    src/imgui_draw.cpp
    src/imgui_tables.cpp
    src/imgui_impl_opengl3.cpp
    src/imgui_impl_glfw.cpp)

target_link_libraries(imgui PRIVATE dl)

target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_include_directories(${BINARY_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

# Link main project to imgui lib
target_link_libraries(${BINARY_NAME} PRIVATE imgui)
