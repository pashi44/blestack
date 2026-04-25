#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h> //type of connection
#include "Gpio.hpp"
#include "Ble_structs.hpp"
// #include <zephyr/bluetooth/uuid.h>

LOG_MODULE_REGISTER(Ble_sample_main, LOG_LEVEL_DBG);

#ifdef __cplusplus__
extern "C" {
#endif

int main(void)
{

#ifdef CONFIG_GPIO

	GPIO::Gpio::gpio_init();
	// GPIO::Gpio::gpio_pulse( &GPIO::Gpio::led_blue, 100);

#endif // DEBUG
	int err = bt_enable(NULL);

	if (err) {
		LOG_ERR("Bluetooth initialization failed\n");
 	return  -ENODEV;
	} else {
		LOG_INF("Ble initalized\n");
	}
	
	 Ble_structs::init();
    err = Ble_structs::start_advertising();
    if (err<0) {
        LOG_ERR("Initial advertising failed\n");
    }

	while (1) {
		k_sleep(K_MSEC(1000));
	}

	return 0;
}

#ifdef __cplusplus__
}

#endif // DEBUG
