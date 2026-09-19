#include <zephyr/kernel.h>
#include "gpio/gpio_interrupt.hpp"
#include "gpio/i2c.hpp"

extern "C" int main(void)
{


    while (true) {
        k_msleep(1000);
    }

    return 0;
}
