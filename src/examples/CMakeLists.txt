cmake_minimum_required(VERSION 3.7)

project(SoftwareRendererExamples)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../renderer)

find_package(OpenMP)
if (OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif ()

find_package(SDL2 REQUIRED)

include(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS})

add_executable(RasterizerTest RasterizerTest.cpp)
target_link_libraries(RasterizerTest ${SDL2_LIBRARIES})

add_executable(VertexProcessorTest VertexProcessorTest.cpp)
target_link_libraries(VertexProcessorTest ${SDL2_LIBRARIES})

add_executable(Crate Crate.cpp ObjData.cpp ObjData.h)
target_link_libraries(Crate ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES})