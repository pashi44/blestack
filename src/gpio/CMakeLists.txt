# SPDX-License-Identifier: Apache-2.0

target_sources(app PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/gpio_interrupt.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/I2c.cpp"
)
