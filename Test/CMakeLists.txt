find_package(Catch2 REQUIRED)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TEST_SOURCES})

add_executable(JungleINTests ${TEST_SOURCES})

set_target_properties(JungleINTests PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)

target_link_libraries(JungleINTests
	PRIVATE JungleINLib
	PRIVATE Catch2::Catch2WithMain
)

# Absolute path to the runtime data dir, so file-based tests do not
# depend on the test working directory.
target_compile_definitions(JungleINTests
	PRIVATE JUNGLEIN_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

include(CTest)
include(Catch)
catch_discover_tests(JungleINTests)
