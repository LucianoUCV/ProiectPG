cmake_minimum_required(VERSION 4.0)
project(ProiectPG)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)

add_executable(ProiectPG main.cpp)

target_include_directories(ProiectPG PRIVATE ${OPENGL_INCLUDE_DIR})
target_link_libraries(ProiectPG ${OPENGL_LIBRARIES} "-framework GLUT")
