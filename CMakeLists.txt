cmake_minimum_required(VERSION 3.7)

project(SoftwareRenderer)

if(EXISTS "${CMAKE_BINARY_DIR}/CMakeTools/CMakeToolsHelpers.cmake")
	include(${CMAKE_BINARY_DIR}/CMakeTools/CMakeToolsHelpers.cmake)
endif()

find_package(SDL2 REQUIRED)
include_directories(SoftwareRenderer ${SDL2_INCLUDE_DIRS})

add_definitions("-ggdb -funroll-loops")

add_executable(SoftwareRenderer Main.cpp)
target_link_libraries(SoftwareRenderer ${SDL2_LIBRARIES})