cmake_minimum_required(VERSION 3.7)

project(SoftwareRenderer)

set(SOURCE_FILES 
	Main.cpp 
	IRasterizer.h
	Rasterizer.h
	VertexProcessor.h)

if (EXISTS "${CMAKE_BINARY_DIR}/CMakeTools/CMakeToolsHelpers.cmake")
	include(${CMAKE_BINARY_DIR}/CMakeTools/CMakeToolsHelpers.cmake)
endif ()

find_package(OpenMP)
if (OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif ()

find_package(SDL2 REQUIRED)
include_directories(SoftwareRenderer ${SDL2_INCLUDE_DIRS})

add_executable(SoftwareRenderer ${SOURCE_FILES})
target_link_libraries(SoftwareRenderer ${SDL2_LIBRARIES})