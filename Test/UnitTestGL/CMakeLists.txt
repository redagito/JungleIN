file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TEST_SOURCES})

add_executable(UnitTestGL ${TEST_SOURCES})

target_compile_features(UnitTestGL PRIVATE cxx_std_23)

target_link_libraries(UnitTestGL
	PRIVATE JungleINLib
	PRIVATE Catch2::Catch2WithMain
)

# Shared test helpers live in UnitTest/source (TestFiles.h); include
# that dir instead of reaching across with a relative ../../ path.
target_include_directories(UnitTestGL
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source
	PRIVATE ${CMAKE_SOURCE_DIR}/Test/UnitTest/source
)

# Absolute path to the runtime data dir, so file-based tests do not
# depend on the test working directory.
target_compile_definitions(UnitTestGL
	PRIVATE JUNGLEIN_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

# The GL binary runs all its cases in one process, serially: a single
# ctest entry, so parallel presets cannot interleave hidden GL contexts
# (concurrent contexts contend on GPU/driver-shared state and flake).
add_test(NAME UnitTestGL COMMAND UnitTestGL)
