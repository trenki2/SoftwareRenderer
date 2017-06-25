cmake_minimum_required(VERSION 3.7)

project(SoftwareRendererExamples)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../renderer)

find_package(SDL2 REQUIRED)
include_directories(SoftwareRenderer ${SDL2_INCLUDE_DIRS})

add_executable(RasterizerTest RasterizerTest.cpp)
target_link_libraries(RasterizerTest ${SDL2_LIBRARIES})

add_executable(VertexProcessorTest VertexProcessorTest.cpp)
target_link_libraries(VertexProcessorTest ${SDL2_LIBRARIES})