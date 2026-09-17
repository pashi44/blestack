#include <zephyr/kernel.h>
#include "gpio/nrf9151_registers.hpp"
#include "gpio/gpio_interrupt.hpp"

extern "C" int main(void)
{
    Gpios::configure_leds();
    Gpios::button_init();




    while (true) {
        k_msleep(1000);
    }

    return 0;
}
