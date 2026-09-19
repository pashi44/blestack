#pragma once
#include <cstdint>

// Keep the existing namespace. Neither backend currently performs an I2C read.
namespace Registers::I2c {
extern uint32_t get_vendor_id();
}
