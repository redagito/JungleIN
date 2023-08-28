cmake_minimum_required(VERSION 3.25.0)
project(JungleIN)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CXX_STANDARD_REQUIRED ON)

find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glad REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(stb REQUIRED)
find_package(OpenAL REQUIRED)
find_package(minimp3 REQUIRED)
find_package(fmtlog REQUIRED)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS 
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cxx
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.hpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties (${PROJECT_NAME} PROPERTIES 
	CXX_STANDARD 17
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE glm::glm
	PRIVATE fmtlog::fmtlog
	PRIVATE OpenAL::OpenAL
	PRIVATE OpenGL::GL
	PRIVATE glfw
	PRIVATE glad::glad
	PRIVATE stb::stb
	PRIVATE minimp3::minimp3
	PRIVATE tinyobjloader::tinyobjloader
)

target_include_directories(${PROJECT_NAME} 
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source
)

# Copy runtime data
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})