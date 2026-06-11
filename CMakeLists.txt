cmake_minimum_required(VERSION 3.20)

project(OpenGL)

add_subdirectory(external/glfw-3.4)

add_executable(
    OpenGL
    src/main.cpp
    src/glad.c
    src/stb_image.cpp
)

set_target_properties(OpenGL PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_include_directories(OpenGL PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(
    OpenGL
    PRIVATE
    glfw
)
