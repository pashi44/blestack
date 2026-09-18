#include "gpio/gpio_interrupt.hpp"
#include "gpio/i2c.hpp"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

// QEMU emulates a Cortex-M3 board, not the nRF9151 peripherals.
// These functions simulate application behavior without Nordic MMIO accesses.
namespace {
void leds_off(k_work *)
{
    printk(" LEDs off\n");
}
K_WORK_DELAYABLE_DEFINE(off_work, leds_off);

void button_press(k_work *);
K_WORK_DELAYABLE_DEFINE(button_work, button_press);
void button_press(k_work *)
{
    printk("[SIM] Simulated button press: LEDs on\n");
    k_work_schedule(&off_work, K_MSEC(1000));
    k_work_schedule(&button_work, K_SECONDS(5));
}
} // namespace

namespace Gpios {
void configure_leds(void)
{
    printk("[SIM] GPIO backend ready; LEDs initially off\n");
}
void button_init(void)
{
    // Workqueue callback, not a hardware interrupt. First press after 2 seconds.
    k_work_schedule(&button_work, K_SECONDS(2));
}
} // namespace Gpios

namespace Registers::I2c {
uint32_t get_vendor_id()
{
    printk("[SIM] I2C placeholder; returning 0xff without bus traffic\n");
    return 0xff;
}
} // namespace Registers::I2c
