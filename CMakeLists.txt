cmake_minimum_required(VERSION 3.5)

project(qnavierstokes VERSION 1.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(OpenGL)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS OpenGL)
find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)
find_package(Qt6 REQUIRED COMPONENTS Help)

add_subdirectory(src/app)
add_subdirectory(src/bottom_heating)
add_subdirectory(src/color_scale)
add_subdirectory(src/fem_tools)
add_subdirectory(src/help)
add_subdirectory(src/matrix_tools)
add_subdirectory(src/opengl_painter)
add_subdirectory(src/settings)
add_subdirectory(src/side_heating)
add_subdirectory(src/slae_solver)

