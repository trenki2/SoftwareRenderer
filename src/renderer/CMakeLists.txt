cmake_minimum_required(VERSION 3.7)

project(renderer)

set(SOURCE_FILES 
	IRasterizer.h
	Rasterizer.h
	VertexProcessor.h)

find_package(OpenMP)
if (OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
	add_definitions("-Wall")
endif ()