cmake_minimum_required(VERSION 3.14)

# Project name and version
project(blake3-sparse VERSION 1.0)

set(CMAKE_BUILD_PARALLEL_LEVEL 4)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)

FetchContent_Declare(
    blake3
    GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3
    GIT_TAG 1.8.2
)

FetchContent_MakeAvailable(blake3)

# Check if the patch is already applied
execute_process(
    COMMAND git apply --reverse --check --ignore-space-change --ignore-whitespace ${CMAKE_CURRENT_SOURCE_DIR}/blake3.patch
    WORKING_DIRECTORY ${blake3_SOURCE_DIR}
    RESULT_VARIABLE PATCH_CHECK_RESULT
    ERROR_VARIABLE PATCH_CHECK_ERROR
    OUTPUT_VARIABLE PATCH_CHECK_OUTPUT
)

if(PATCH_CHECK_RESULT EQUAL 0)
    message(STATUS "Patch already applied, skipping.")
else()
    message(STATUS "Applying blake3.patch...")
    execute_process(
        COMMAND git apply --ignore-space-change --ignore-whitespace ${CMAKE_CURRENT_SOURCE_DIR}/blake3.patch
        WORKING_DIRECTORY ${blake3_SOURCE_DIR}
        RESULT_VARIABLE PATCH_RESULT
        ERROR_VARIABLE PATCH_ERROR
        OUTPUT_VARIABLE PATCH_OUTPUT
    )

    if(NOT PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to apply patch:\n${PATCH_ERROR}")
    endif()

endif()

add_subdirectory(${blake3_SOURCE_DIR}/c blake3)

add_library(blake3z STATIC dummy.c)
target_link_libraries(blake3z PUBLIC blake3)

add_library(blake3z_file STATIC blake3z_file.cpp)
target_link_libraries(blake3z_file PUBLIC blake3z)
