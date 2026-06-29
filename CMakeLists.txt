cmake_minimum_required(VERSION 3.20)

project(OpenGL)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_subdirectory(external/glfw-3.4)

add_executable(
    OpenGL
    src/main.cpp
    src/glad.c
    src/rendering.cpp
    src/stb_image.cpp
    src/world.cpp
    src/chunk.cpp
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
