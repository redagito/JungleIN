file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${APP_SOURCES})

add_executable(JungleINApp ${APP_SOURCES})

set_target_properties(JungleINApp PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
)

target_link_libraries(JungleINApp
	PRIVATE JungleINLib
)

# Runtime data (shaders, textures, heightmaps) is loaded via relative
# "data/..." paths, so it must sit next to the executable at run time.
file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(TARGET JungleINApp POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:JungleINApp>/data
)
