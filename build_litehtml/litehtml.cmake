set(LITEHTML_COMMIT f358a56a5d537e6f815e267fe8cab5cca1c742cb)

FetchContent_Declare(litehtml-custom
	GIT_REPOSITORY https://github.com/litehtml/litehtml.git
	GIT_TAG ${LITEHTML_COMMIT}
	GIT_PROGRESS true
	SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/litehtml-download"
	SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/litehtml-src"
	BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/litehtml-build"
)

set(LITEHTML_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(LITEHTML_ENABLE_LINT OFF CACHE BOOL "" FORCE)
set(EXTERNAL_GUMBO OFF CACHE BOOL "" FORCE)

set(LITEHTML_SAVED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(litehtml-custom)
set(BUILD_SHARED_LIBS ${LITEHTML_SAVED_BUILD_SHARED_LIBS})

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang")
	target_compile_options(litehtml PRIVATE -fno-fast-math -w)
	target_compile_options(gumbo PRIVATE -fno-fast-math -w)
endif()
