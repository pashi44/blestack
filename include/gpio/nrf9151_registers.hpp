#pragma once
#include <cstdint>

// nRF9151 Product Specification v1.0:
// peripheral IDs/IRQs: pp. 25-26; GPIO registers: section 6.4.4;
// GPIOTE registers: section 6.5.4. These are MMIO addresses, not RAM buffers.

namespace Registers {

inline volatile uint32_t &at(uintptr_t address) {
    return *reinterpret_cast<volatile uint32_t *>(address);
}

namespace Gpio {

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
constexpr uintptr_t p0_base = 0x40842500UL;
constexpr uintptr_t gpiote_base = 0x40031000UL; // GPIOTE1, nonsecure
constexpr unsigned gpiote_irq = 49U;
#elif (CONFIG_TRUSTED_EXECUTION_SECURE)
constexpr uintptr_t p0_base = 0x50842500UL;
constexpr uintptr_t gpiote_base = 0x5000D000UL; // GPIOTE0, secure
constexpr unsigned gpiote_irq = 13U;
#endif

constexpr uint32_t outset = 0x008UL;
constexpr uint32_t outclr = 0x00CUL;
constexpr uint32_t dirset = 0x018UL;
constexpr uint32_t pin_cnf = 0x200UL; // configuration register at 0x2000 offset from the base
constexpr uint32_t events_port = 0x17CUL;
constexpr uint32_t intenset = 0x304UL;
constexpr uint32_t intenclr = 0x308UL;
constexpr uint32_t port_interrupt_mask = 1UL << 31;

/* The Dangerous NVIC and GPIOTE register to trigger the interrupts */
constexpr uint32_t nvic_base = 0xE000E100UL;
constexpr uint32_t nvic_IPR0 = nvic_base + 0x300UL; // Interrupt Priority Register 0

} // namespace Gpio

namespace I2c {

// share same as the Uart0 
constexpr uint32_t i2c0master_s_base = 0x50008000UL;  
constexpr uint32_t i2c0master_ns_base = 0x40008000UL;
constexpr uint32_t i2c1master_s_base = 0x50009000UL;  
constexpr uint32_t i2c1master_ns_base = 0x40009000UL;
constexpr uint32_t i2c2master_s_base = 0x5000A000UL;  
constexpr uint32_t i2c2master_ns_base = 0x4000A000UL;
constexpr uint32_t i2c3master_s_base = 0x5000B000UL;  
constexpr uint32_t i2c3master_ns_base = 0x4000B000UL;

extern uint32_t get_vendor_id();

} // namespace I2c
} // namespace Registers