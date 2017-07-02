cmake_minimum_required(VERSION 3.7)

project(SoftwareRendererExamples)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../renderer)

find_package(OpenMP)
if (OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif ()

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

add_executable(RasterizerTest RasterizerTest.cpp)
target_link_libraries(RasterizerTest renderer ${SDL2_LIBRARIES})

add_executable(VertexProcessorTest VertexProcessorTest.cpp)
target_link_libraries(VertexProcessorTest renderer ${SDL2_LIBRARIES})

add_executable(Box Box.cpp ObjData.cpp ObjData.h)
target_link_libraries(Box renderer ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})