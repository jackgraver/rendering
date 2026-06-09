cmake_minimum_required(VERSION 3.20)

project(OpenGL)

add_subdirectory(external/glfw-3.4)

add_executable(
    OpenGL
    src/main.cpp
    src/glad.c
)

target_include_directories(OpenGL PRIVATE
    $(CMAKE_SOURCE_DIR)/include
)

target_link_libraries(
    OpenGL
    PRIVATE
    glfw
)
