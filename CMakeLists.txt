# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)




set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DTC_OVERLAY_FILE)
  set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/boards/nordic/nrf9151/nrf9151dk.overlay")
endif()
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(peripheral)

# Keep the application entry point here; modules register their own sources.
target_sources(app PRIVATE src/main.cpp)
add_subdirectory(include)
add_subdirectory(src)
