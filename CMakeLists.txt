cmake_minimum_required(VERSION 3.7)

project(SoftwareRenderer)

set(SOURCE_FILES 
	Main.cpp 
	Rasterizer.h)

if(EXISTS "${CMAKE_BINARY_DIR}/CMakeTools/CMakeToolsHelpers.cmake")
	include(${CMAKE_BINARY_DIR}/CMakeTools/CMakeToolsHelpers.cmake)
endif()

find_package(SDL2 REQUIRED)
include_directories(SoftwareRenderer ${SDL2_INCLUDE_DIRS})

add_executable(SoftwareRenderer ${SOURCE_FILES})
target_link_libraries(SoftwareRenderer ${SDL2_LIBRARIES})