# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Board-named overlays are discovered by Zephyr; QEMU gets no Nordic overlay.
# DTC_OVERLAY_FILE can still be provided explicitly by the caller.

# Installation paths belong to each machine, not to this repository.
# Accept -DZEPHYR_BASE=... or ZEPHYR_BASE from the environment; registered
# Zephyr packages are also supported. Quote paths on all host platforms.
# Let Zephyr select the cross compiler using BOARD and its toolchain settings
# (e.g. ZEPHYR_TOOLCHAIN_VARIANT and ZEPHYR_SDK_INSTALL_DIR).
find_package(Zephyr REQUIRED HINTS "${ZEPHYR_BASE}" "$ENV{ZEPHYR_BASE}")
project(peripheral LANGUAGES C CXX ASM)

# Keep the application entry point here; modules register their own sources.
target_sources(app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
add_subdirectory(include)
add_subdirectory(src)
