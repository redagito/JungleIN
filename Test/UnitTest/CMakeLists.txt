file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TEST_SOURCES})

add_executable(UnitTest ${TEST_SOURCES})

set_target_properties(UnitTest PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
)

target_link_libraries(UnitTest
	PRIVATE JungleINLib
	PRIVATE Catch2::Catch2WithMain
)

# Absolute path to the runtime data dir, so file-based tests do not
# depend on the test working directory.
target_compile_definitions(UnitTest
	PRIVATE JUNGLEIN_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

catch_discover_tests(UnitTest)
