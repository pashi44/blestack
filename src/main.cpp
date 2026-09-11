#include <zephyr/kernel.h>
#include "gpio/gpio_interrupt.hpp"

extern "C" int main(void)
{
    gpio::leds_configure_as_output();
    gpio::button_init();

    while (true) {
        k_msleep(1000);
    }

    return 0;
}
