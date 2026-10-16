file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${APP_SOURCES})

add_executable(JungleINApp ${APP_SOURCES})

target_compile_features(JungleINApp PRIVATE cxx_std_23)

target_link_libraries(JungleINApp
	PRIVATE JungleINLib
)

# Runtime data (shaders, textures, heightmaps) is loaded via relative
# "data/..." paths, so it must sit next to the executable at run time.
# Single POST_BUILD copy: it runs after every build straight into the
# per-config output dir, so configure-time copies never go stale.
add_custom_command(TARGET JungleINApp POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:JungleINApp>/data
)
