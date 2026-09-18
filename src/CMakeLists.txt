# SPDX-License-Identifier: Apache-2.0

# BOARD selects the implementation at configure time. Only one backend links.
if(CONFIG_BOARD_NRF9151DK)
  add_subdirectory(gpio)
elseif(CONFIG_BOARD_QEMU_CORTEX_M3)
  add_subdirectory(qemu)
else()
  message(FATAL_ERROR "Supported boards: nrf9151dk/nrf9151/ns and qemu_cortex_m3")
endif()
