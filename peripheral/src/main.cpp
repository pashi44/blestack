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
	GPIO::Gpio::gpio_pulse(10);

#endif // DEBUG
	int err = bt_enable(NULL);

	if (err) {
		LOG_ERR("Bluetooth initialization failed\n");
 	return  -ENODEV;
	} else {
		LOG_INF("Ble initalized\n");
		err = Ble_structs::Randomize_address();
	}

	err = Ble_structs::start_advertising();

	while (1) {
		LOG_INF("spitting he serial signals fo rble\n");

		// Ble_structs::adv_mfg_config.custom_data++;
		//  k_sleep(K_MSEC(100));
		k_sleep(K_MSEC(1000));
	}

	return 0;
}

#ifdef __cplusplus__
}

#endif // DEBUG
