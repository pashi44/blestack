#include <zephyr/kernel.h>
#include "gpio/gpio_interrupt.hpp"

extern "C" int main(void)
{
    Gpios::configure_leds();
    Gpios::button_init();
printk("%u\n", Registers::I2c::get_vendor_id());







    while (true) {
        k_msleep(1000);
    }

    return 0;
}
