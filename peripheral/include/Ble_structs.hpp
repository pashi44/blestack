
#ifndef BLE_STRUCTS_HPP
#define BLE_STRUCTS_HPP
#if defined(CONFIG_BT)
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME

#define COMPANY_ID_CODE (0x0059)

#endif // CONFIG_BT

#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT)

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/random/random.h>
#endif

#if defined(CONFIG_PWM) && defined(CONFIG_GPIO)
#include "Gpio.hpp"
#endif //

#if defined(CONFIG_BT) || defined(CONFIG_BT_PERIPHERAL)

/* K_WORK_DEFINE(adv_work, Ble_structs::adv_work_handler);
	struct k_work work = Z_WORK_INITIALIZER(work_handler)
*/
struct Ble_structs {
	static inline const uint8_t ad_flags = BT_LE_AD_NO_BREDR;
	

	Ble_structs() = delete;
	Ble_structs(const Ble_structs &) = delete;
	Ble_structs &operator=(const Ble_structs &) = delete;

	struct adv_mfg_config {
		uint16_t company_id;
		uint16_t custom_data;
	};
	static inline struct Ble_structs::adv_mfg_config adv_mfg_config = {COMPANY_ID_CODE, 0x00};

	static inline const struct bt_data ad[] = {{
							   .type = (uint8_t)BT_DATA_FLAGS,
							   .data_len = (uint8_t)sizeof(ad_flags),
							   .data = &ad_flags,
						   },
						   {
							   .type = (uint8_t)BT_DATA_NAME_COMPLETE,
							   .data_len = sizeof(DEVICE_NAME) - 1,
							   .data = (uint8_t *)DEVICE_NAME,
						   },
						   {.type = (uint8_t)BT_DATA_MANUFACTURER_DATA,
						    .data_len = sizeof(adv_mfg_config),
						    .data = (uint8_t *)&adv_mfg_config}};

	static inline struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.sid = 0,
		.secondary_max_skip = 0,
		.options = BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY,
		.interval_min = 80,
		.interval_max = 160,
		.peer = NULL,
	};

	static inline unsigned char mygithub[] = {0x17, '/', '/', 'g', 'i', 't', 'h',
						  'u',  'b', '.', 'c', 'o', 'm', '/',
						  'p',  'a', 's', 'h', 'i', '4', '4'};
	static inline const struct bt_data scan_response[] = {
		BT_DATA(BT_DATA_URI, Ble_structs::mygithub, sizeof(Ble_structs::mygithub)),
	};

	static inline bt_addr_le_t addr;
	static inline int random_id = BT_ID_DEFAULT;

#if defined(CONFIG_BT) && defined(CONFIG_BT_ID_MAX)
	static int Randomize_address()
	{
		uint32_t ran = sys_rand32_get();
		char addr_str[18];
		snprintf(addr_str, sizeof(addr_str), "D2:44:33:22:11:%02X", (uint8_t)(ran & 0xFF));

		int err = bt_addr_le_from_str(addr_str, "random", &Ble_structs::addr);
		if (err) {
			return -EINVAL;
		}

		bt_id_reset(ran, NULL, NULL);

		err = bt_id_create(&Ble_structs::addr, NULL);
		if (err < 0) {
			return err;
		}

		random_id = err;
		Ble_structs::adv_param.id = Ble_structs::random_id;
		GPIO::Gpio::pulse_all(5000);
		return 0;
	}

#endif

#if defined(CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE) && defined(CONFIG_BT_PERIPHERAL)
	/* State and callbacks */
	static inline struct k_work adv_work;
	
	static int start_advertising(void)
	{
		int err = bt_le_adv_start(&Ble_structs::adv_param, Ble_structs::ad,
			ARRAY_SIZE(Ble_structs::ad), Ble_structs::scan_response,
			ARRAY_SIZE(Ble_structs::scan_response));
			if (err < 0) {
				return -ENODEV;
			}else  GPIO::Gpio::gpio_pulse(&GPIO::Gpio::led_blue, 500);
			return err;
		}
#if  defined(CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE) && defined(CONFIG_BT_PERIPHERAL)
	static void on_disconnected(struct bt_conn *conn, uint8_t reason)
	{
		printk("Disconnected (reason 0x%02x). Scheduling re-advertising...\n", reason);
         GPIO::Gpio::gpio_pulse(&GPIO::Gpio::led_red, 500);
		k_work_submit(&Ble_structs::adv_work);
	}

	static void adv_work_handler(struct k_work *work)
	{
		bt_le_adv_stop();
		Randomize_address();

		int err = Ble_structs::start_advertising();
		if (err < 0) {
			printk("advertisment failed from work handler\n");
		} else {
			printk("advertisment started with new MAC ID: %d\n", random_id);
		}
	}

	static void recycled_cb(void)
	{
		printk("Connection object recycled (memory freed).\n");
	}

	static void on_connected(struct bt_conn *conn, uint8_t err)
	{
		if (err) {
			printk("Connection failed (err 0x%02x)\n", err);
			return;
		}
	GPIO::Gpio::gpio_pulse(&GPIO::Gpio::led_green, 500);

	}

	static inline struct bt_conn_cb conn_callbacks = {
		.connected = on_connected,
		.disconnected = on_disconnected,
		.recycled = recycled_cb,
	};



	static void init()
	{
		k_work_init(&adv_work, adv_work_handler);
		bt_conn_cb_register(&conn_callbacks);
	}
	#endif // DEBUG

#endif
};

#endif
#endif
