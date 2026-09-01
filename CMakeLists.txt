# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# 1. Tell CMake where the Zephyr repository is located
set(${ZEPHYR_BASE} ZEPHYR_BASE "/home/hadoop/zephyrworkspace/zephyr")

set(${CMAKE_SDK_INSTALL_DIR} ZEPHYR_SDK_INSTALL_DIR "/home/hadoop/.zephyr_ide/toolchains/zephyr-sdk-1.0.1")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(peripheral)

target_sources(app PRIVATE
  src/main.cpp
)