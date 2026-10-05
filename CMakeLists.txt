cmake_minimum_required(VERSION 3.20)

project(graphics_lab VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

# GLAD 0.1.36 was generated for an OpenGL 3.3 compatibility profile.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

add_library(graphics_lab_glad STATIC
    glad/src/glad.c
)

target_include_directories(graphics_lab_glad
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/glad/include"
)

target_compile_features(graphics_lab_glad PUBLIC c_std_99)

# Link this target to an application that uses modern OpenGL.
add_library(graphics_lab_options INTERFACE)

target_compile_features(graphics_lab_options INTERFACE cxx_std_17)
target_link_libraries(graphics_lab_options
    INTERFACE
        graphics_lab_glad
        glfw
        OpenGL::GL
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(graphics_lab
        src/main.cpp
    )

    target_link_libraries(graphics_lab
        PRIVATE
            graphics_lab_options
    )

    target_compile_definitions(graphics_lab
        PRIVATE
            GRAPHICS_LAB_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    )

    if(MSVC)
        target_compile_options(graphics_lab PRIVATE /W4 /permissive-)
    else()
        target_compile_options(graphics_lab PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()