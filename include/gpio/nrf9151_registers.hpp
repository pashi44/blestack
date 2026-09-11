#pragma once

#include <cstdint>

// nRF9151 Product Specification v1.0:
// peripheral IDs/IRQs: pp. 25-26; GPIO registers: section 6.4.4;
// GPIOTE registers: section 6.5.4. These are MMIO addresses, not RAM buffers.
namespace gpio {
namespace registers {

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
constexpr uintptr_t p0_base = 0x40842500UL;
constexpr uintptr_t gpiote_base = 0x40031000UL; // GPIOTE1, nonsecure
constexpr unsigned gpiote_irq = 49U;
#else
constexpr uintptr_t p0_base = 0x50842500UL;
constexpr uintptr_t gpiote_base = 0x5000D000UL; // GPIOTE0, secure
constexpr unsigned gpiote_irq = 13U;
#endif

constexpr uintptr_t outset = 0x008UL;
constexpr uintptr_t outclr = 0x00CUL;
constexpr uintptr_t dirset = 0x018UL;
constexpr uintptr_t pin_cnf = 0x200UL; // PIN_CNF[n] = base + 0x200 + n * 4
constexpr uintptr_t events_port = 0x17CUL;
constexpr uintptr_t intenset = 0x304UL;
constexpr uintptr_t intenclr = 0x308UL;
constexpr uint32_t port_interrupt_mask = 1UL << 31;

inline volatile uint32_t &at(uintptr_t address)
{
    return *reinterpret_cast<volatile uint32_t *>(address);
}

} // namespace registers
} // namespace gpio
