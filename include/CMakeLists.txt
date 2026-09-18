# SPDX-License-Identifier: Apache-2.0

# Resolve headers relative to this checkout, independent of the host machine.
target_include_directories(app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
