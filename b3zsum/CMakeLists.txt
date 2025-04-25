cmake_minimum_required(VERSION 3.14)

# Project name and version
project(blake3-sparse2 VERSION 1.0)

set(CMAKE_BUILD_PARALLEL_LEVEL 4)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_BUILD_TYPE Release)

set(DEFAULT_CACHE_PATHNAME "{exe_path}/blake3z.cache" CACHE STRING "Default cache file pathname")
set(B3ZSUM_SOURCES
    b3zsum.cpp
)

include(FetchContent)

FetchContent_Declare(
    blake3z
    GIT_REPOSITORY https://github.com/zed-0xff/blake3z
)

FetchContent_MakeAvailable(blake3z)

add_executable(b3zsum ${B3ZSUM_SOURCES})
target_include_directories(b3zsum PRIVATE ${blake3z_SOURCE_DIR})
target_link_libraries(b3zsum PRIVATE blake3z_file)
target_compile_definitions(b3zsum PRIVATE DEFAULT_CACHE_PATHNAME="${DEFAULT_CACHE_PATHNAME}")

if (NOT APPLE)
    # Statically linked executable
    add_executable(b3zsum_static ${B3ZSUM_SOURCES})
    target_include_directories(b3zsum_static PRIVATE ${blake3z_SOURCE_DIR})
    target_link_libraries(b3zsum_static PRIVATE blake3z_file -static)
    target_compile_definitions(b3zsum_static PRIVATE DEFAULT_CACHE_PATHNAME="${DEFAULT_CACHE_PATHNAME}")

    # Enforce static linking flags
    set_target_properties(b3zsum_static PROPERTIES
        LINK_SEARCH_START_STATIC TRUE
        LINK_SEARCH_END_STATIC TRUE
    )
endif()
