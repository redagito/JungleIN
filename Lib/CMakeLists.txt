find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glad REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(stb REQUIRED)
find_package(OpenAL REQUIRED)
find_package(minimp3 REQUIRED)
find_package(fmtlog REQUIRED)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cxx
)
file(GLOB_RECURSE LIB_HEADERS CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${LIB_SOURCES} ${LIB_HEADERS})

add_library(JungleINLib STATIC ${LIB_SOURCES} ${LIB_HEADERS})

set_target_properties(JungleINLib PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
)

# Public: consumers (App/Test) include Lib headers as e.g. "ui/Window.h".
# Only the include/ root is exposed; per-subdir entries were removed
# because they shadow system math.h via Math.h on case-insensitive
# Windows and caused cmath build errors.
target_include_directories(JungleINLib
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(JungleINLib
	PUBLIC
		glm::glm
		fmtlog::fmtlog
		OpenAL::OpenAL
		OpenGL::GL
		glfw
		glad::glad
		stb::stb
		minimp3::minimp3
		tinyobjloader::tinyobjloader
)
