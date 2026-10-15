cmake_minimum_required(VERSION 4.4)
project(JungleIN VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Warnings as errors, warning level raised stepwise (W1 -> W4).
# /external:W0 keeps third-party headers (marked SYSTEM where consumed)
# out of our warning budget.
if (MSVC)
	add_compile_options(/WX /W4 /external:W0)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT JungleINApp)

enable_testing()

add_subdirectory(Lib)
add_subdirectory(App)
add_subdirectory(Test)
