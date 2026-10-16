file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TEST_SOURCES})

add_executable(UnitTestGL ${TEST_SOURCES})

set_target_properties(UnitTestGL PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
)

target_link_libraries(UnitTestGL
	PRIVATE JungleINLib
	PRIVATE Catch2::Catch2WithMain
)

# Absolute path to the runtime data dir, so file-based tests do not
# depend on the test working directory.
target_compile_definitions(UnitTestGL
	PRIVATE JUNGLEIN_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

# The GL binary runs all its cases in one process, serially: a single
# ctest entry, so parallel presets cannot interleave hidden GL contexts
# (concurrent contexts contend on GPU/driver-shared state and flake).
add_test(NAME GLTests COMMAND UnitTestGL)
