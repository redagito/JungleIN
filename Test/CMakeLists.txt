find_package(Catch2 REQUIRED)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TEST_SOURCES})

add_executable(JungleINTests ${TEST_SOURCES})

# GL-context cases live in their own binary, registered as ONE serial
# ctest entry below: concurrent hidden contexts across parallel test
# processes contend on GPU/driver-shared state and flake intermittently
# (different victims, green alone and serial). The two CPU-only cases
# inside these files (Mesh locations, tilted-plane math) ride along;
# splitting files per case is not worth the surgery.
set(GL_TEST_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/source/TestShaderCompile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/source/TestOverlayRender.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/source/TestUniformRegistration.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/source/TestTextureUnits.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/source/TestFailFastGL.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/source/TestBatchB.cpp
)
list(REMOVE_ITEM TEST_SOURCES ${GL_TEST_SOURCES})

add_executable(JungleINGLTests ${GL_TEST_SOURCES})

set_target_properties(JungleINTests PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
)

set_target_properties(JungleINGLTests PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
)

target_link_libraries(JungleINTests
	PRIVATE JungleINLib
	PRIVATE Catch2::Catch2WithMain
)

target_link_libraries(JungleINGLTests
	PRIVATE JungleINLib
	PRIVATE Catch2::Catch2WithMain
)

# Absolute path to the runtime data dir, so file-based tests do not
# depend on the test working directory.
target_compile_definitions(JungleINTests
	PRIVATE JUNGLEIN_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

target_compile_definitions(JungleINGLTests
	PRIVATE JUNGLEIN_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

include(CTest)
include(Catch)
catch_discover_tests(JungleINTests)

# The GL binary runs all its cases in one process, serially: a single
# ctest entry, so parallel presets cannot interleave GL contexts.
add_test(NAME GLTests COMMAND JungleINGLTests)
