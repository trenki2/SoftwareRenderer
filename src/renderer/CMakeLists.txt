cmake_minimum_required(VERSION 3.7)

project(renderer)

set(SOURCE_FILES
	Renderer.h
	EdgeData.h
	EdgeEquation.h
	IRasterizer.h
	LineClipper.cpp
	LineClipper.h
	ParameterEquation.h
	PixelData.h
	PixelShaderBase.h
	PolyClipper.cpp
	PolyClipper.h
	Rasterizer.h
	TriangleEquations.h
	VertexCache.h
	VertexConfig.h
	VertexProcessor.cpp
	VertexProcessor.h
	VertexShaderBase.h)

find_package(OpenMP)
if (OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
	add_definitions("-Wall")
endif ()

add_library(renderer ${SOURCE_FILES})