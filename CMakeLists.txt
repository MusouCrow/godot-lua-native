cmake_minimum_required(VERSION 3.17)

project(godot-lua-native
				VERSION 0.1.0
				DESCRIPTION "Lua 5.5 integration for Godot 4.x via GDExtension"
				LANGUAGES C CXX
)

# Build options
set(GODOTCPP_TARGET "template_debug" CACHE STRING "Godot target: template_debug, template_release, editor")

# Add godot-cpp as subdirectory
add_subdirectory(godot-cpp)

# Get godot-cpp properties
get_target_property(GODOTCPP_SUFFIX godot-cpp GODOTCPP_SUFFIX)
get_target_property(GODOTCPP_PLATFORM godot-cpp GODOTCPP_PLATFORM)

# Lua library (build from source)
set(LUA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua)

file(GLOB LUA_CORE_SOURCES
				${LUA_DIR}/lapi.c
				${LUA_DIR}/lcode.c
				${LUA_DIR}/lctype.c
				${LUA_DIR}/ldebug.c
				${LUA_DIR}/ldo.c
				${LUA_DIR}/ldump.c
				${LUA_DIR}/lfunc.c
				${LUA_DIR}/lgc.c
				${LUA_DIR}/llex.c
				${LUA_DIR}/lmem.c
				${LUA_DIR}/lobject.c
				${LUA_DIR}/lopcodes.c
				${LUA_DIR}/lparser.c
				${LUA_DIR}/lstate.c
				${LUA_DIR}/lstring.c
				${LUA_DIR}/ltable.c
				${LUA_DIR}/ltm.c
				${LUA_DIR}/lundump.c
				${LUA_DIR}/lvm.c
				${LUA_DIR}/lzio.c
)

file(GLOB LUA_LIB_SOURCES
				${LUA_DIR}/lauxlib.c
				${LUA_DIR}/lbaselib.c
				${LUA_DIR}/lcorolib.c
				${LUA_DIR}/ldblib.c
				${LUA_DIR}/liolib.c
				${LUA_DIR}/lmathlib.c
				${LUA_DIR}/loadlib.c
				${LUA_DIR}/loslib.c
				${LUA_DIR}/lstrlib.c
				${LUA_DIR}/ltablib.c
				${LUA_DIR}/lutf8lib.c
				${LUA_DIR}/linit.c
)

add_library(lua STATIC ${LUA_CORE_SOURCES} ${LUA_LIB_SOURCES})
target_include_directories(lua PUBLIC ${LUA_DIR})

# Platform-specific Lua settings
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_compile_definitions(lua PRIVATE LUA_USE_LINUX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_compile_definitions(lua PRIVATE LUA_USE_MACOSX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# No special definitions needed for Windows
endif()

# Position independent code for shared library linking
set_target_properties(lua PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Extension library
set(EXTENSION_NAME "luagd")

set(EXTENSION_SOURCES
				src/lua/lua_runtime.cpp
				src/host/host_thread_check.cpp
				src/host/lua_host.cpp
				src/host/gdextension_entry.cpp
				src/modules/display_module.cpp
				src/modules/core_module.cpp
				src/modules/input_module.cpp
				src/modules/system_module.cpp
				src/modules/audio_module.cpp
				src/modules/node_module.cpp
)

add_library(${EXTENSION_NAME} SHARED ${EXTENSION_SOURCES})

target_include_directories(${EXTENSION_NAME} PRIVATE
				${CMAKE_CURRENT_SOURCE_DIR}/src
				${LUA_DIR}
)

target_link_libraries(${EXTENSION_NAME} PRIVATE
				godot-cpp
				lua
)

set_target_properties(${EXTENSION_NAME} PROPERTIES
				CXX_STANDARD 17
				CXX_EXTENSIONS OFF
				POSITION_INDEPENDENT_CODE ON
				PREFIX "lib"
				OUTPUT_NAME "${EXTENSION_NAME}${GODOTCPP_SUFFIX}"
)

# Output directory
set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../project/addons/luagd)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	# macOS: output with .dylib suffix
	set_target_properties(${EXTENSION_NAME} PROPERTIES
								LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
								SUFFIX ".dylib"
				)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set_target_properties(${EXTENSION_NAME} PROPERTIES
								LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
								RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
				)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set_target_properties(${EXTENSION_NAME} PROPERTIES
								LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
				)
endif()
