﻿cmake_minimum_required (VERSION 3.15)

project (nsl)

add_library(nsl INTERFACE)
add_library(nsl::nsl ALIAS nsl)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(
  nsl
  INTERFACE
    $<BUILD_INTERFACE:${INCLUDE_DIR}>
)

# I stole this mechanism from https://github.com/rollbear/trompeloeil/blob/09e013bc0026630f6e721c1ba61a42aa6256d5eb/CMakeLists.txt#L32
# It allows us to be included gracefully in another project via CMake FetchContent
set(MASTER_PROJECT OFF)
if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  set(MASTER_PROJECT ON)
endif()

# Include sub-projects.
if (MASTER_PROJECT)
  add_subdirectory ("test")
endif() # MASTER_PROJECT
