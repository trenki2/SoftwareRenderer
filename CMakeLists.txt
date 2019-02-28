cmake_minimum_required(VERSION 3.7)

project("SoftwareRenderer")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if (EXISTS "${CMAKE_BINARY_DIR}/CMakeTools/CMakeToolsHelpers.cmake")
	include(${CMAKE_BINARY_DIR}/CMakeTools/CMakeToolsHelpers.cmake)
endif ()

find_package(Doxygen)
if (DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif ()

add_subdirectory(src)