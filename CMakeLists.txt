# Minimum required
# TODO Can we set this lower?
cmake_minimum_required(VERSION 2.8.12)

# Project identifier
project(junglein)


# Required packages
# OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
set(LIBS ${LIBS} ${OPENGL_gl_LIBRARY})


# Boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.55 REQUIRED system program_options filesystem)
include_directories(${Boost_INCLUDE_DIR})
set(LIBS ${LIBS} ${Boost_LIBRARIES})


# Libraries


# Add lodepng
file(GLOB LODEPNG_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/external/lodepng/lodepng.cpp)
set(THIRD_PARTY_SOURCES ${THIRD_PARTY_SOURCES} ${LODEPNG_SOURCES})
include_directories(external/lodepng)


# tinyobjloader
add_subdirectory(external/tinyobjloader)
include_directories(external/tinyobjloader)
set(LIBS ${LIBS} tinyobjloader)


# Flextgl 4.3 core
file(GLOB FLEXTGL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/external/flextgl43/*.cpp)
set(THIRD_PARTY_SOURCES ${THIRD_PARTY_SOURCES} ${FLEXTGL_SOURCES})
include_directories(external/flextgl43)


# GLFW
# Options
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include)
set(LIBS ${LIBS} ${GLFW_LIBRARIES})
set(LIBS ${LIBS} glfw)


# Flags
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
endif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")


# Includes
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/source/
)

# Sources
file(GLOB_RECURSE ENGINE_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.h
)

# Add third party sources
set(ENGINE_SRC_FILES ${ENGINE_SRC_FILES} ${THIRD_PARTY_SOURCES})

# Engine executable
add_executable(junglein ${ENGINE_SRC_FILES})

# Require C++11
set_property(TARGET junglein PROPERTY CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED ON)

# Required libraries
target_link_libraries(junglein ${LIBS})

# Copy data
file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})