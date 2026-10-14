# Aracdia native UI — RmlUi via FetchContent (LGPL, same as Luanti).
#
# Produces:
#   ARACDIA_RMLUI_SOURCES   — GL3 renderer backend (compiled into client)
#   ARACDIA_RMLUI_LIBRARIES — rmlui_core (+ freetype already on client)
#   ARACDIA_RMLUI_INCLUDES  — RmlUi + Backends include dirs

include(FetchContent)

set(RMLUI_TAG "6.0" CACHE STRING "RmlUi git tag for Aracdia UI")

message(STATUS "Fetching RmlUi (${RMLUI_TAG}) for Aracdia native UI...")
FetchContent_Declare(
	RmlUi
	GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
	GIT_TAG        ${RMLUI_TAG}
	GIT_SHALLOW    TRUE
	GIT_PROGRESS   TRUE
)

set(RMLUI_SAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(RMLUI_FONT_ENGINE "freetype" CACHE STRING "" FORCE)
set(RMLUI_LUA_BINDINGS OFF CACHE BOOL "" FORCE)
set(RMLUI_TRACY_PROFILING OFF CACHE BOOL "" FORCE)
set(RMLUI_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(RmlUi)

set(ARACDIA_RMLUI_INCLUDES
	"${rmlui_SOURCE_DIR}/Include"
	"${rmlui_SOURCE_DIR}/Backends"
)

set(ARACDIA_RMLUI_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/client/aracdia/rmlui_renderer_gl2_luanti.cpp"
	"${rmlui_SOURCE_DIR}/Backends/RmlUi_Renderer_GL3.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/client/aracdia/rmlui_system_interface.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/client/aracdia/rmlui_input.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/client/aracdia/rmlui_manager.cpp"
)

set(ARACDIA_RMLUI_LIBRARIES rmlui_core)

if(UNIX AND NOT EMSCRIPTEN)
	list(APPEND ARACDIA_RMLUI_LIBRARIES ${CMAKE_DL_LIBS})
endif()

message(STATUS "Aracdia RmlUi enabled (tag ${RMLUI_TAG})")
