cmake_minimum_required(VERSION 3.14)

# Project name and version
project(blake3-sparse2 VERSION 1.0)

set(CMAKE_BUILD_PARALLEL_LEVEL 4)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_BUILD_TYPE Release)

include(FetchContent)

FetchContent_Declare(
    blake3z
    GIT_REPOSITORY https://github.com/zed-0xff/blake3z
)

FetchContent_MakeAvailable(blake3z)

add_executable(blake3z_gen_cache gen_cache.cpp)
target_link_libraries(blake3z_gen_cache PUBLIC blake3z)
