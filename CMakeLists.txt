# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

# Export the real Zephyr compiler flags for editor IntelliSense.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(peripheral)

# Keep the application entry point here; modules register their own sources.
target_sources(app PRIVATE src/main.cpp)
add_subdirectory(include)
add_subdirectory(src)
