cmake_minimum_required(VERSION 3.7)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if (EXISTS "${CMAKE_BINARY_DIR}/CMakeTools/CMakeToolsHelpers.cmake")
	include(${CMAKE_BINARY_DIR}/CMakeTools/CMakeToolsHelpers.cmake)
endif ()

add_subdirectory(src)