
#ifndef BLE_STRUCTS_HPP
#define BLE_STRUCTS_HPP
// #include <zephyr/logging/log.h>
// LOG_MODULE_DECLARE(Ble_structs, LOG_LEVEL_DBG);

#if defined(CONFIG_BT)
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/logging/log.h>
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME

#define COMPANY_ID_CODE (0x0059)

#endif // CONFIG_BT

#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT)

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/addr.h>
#endif

#if defined(CONFIG_PWM) && defined(CONFIG_GPIO)
#include "Gpio.hpp"
#endif //

#if defined(CONFIG_BT) || defined(CONFIG_BT_PERIPHERAL)

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
	static inline const struct bt_data ad[] = {
		{

			.type = (uint8_t)BT_DATA_FLAGS,
			.data_len = (uint8_t)sizeof(ad_flags),
			.data = &ad_flags,
		},

		// BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_NO_BREDR)),
		//
		{
			.type = (uint8_t)BT_DATA_NAME_COMPLETE,
			.data_len = sizeof(DEVICE_NAME) - 1,
			.data = (uint8_t *)DEVICE_NAME,
		},

		{.type = (uint8_t)BT_DATA_MANUFACTURER_DATA, // 0xff
		 .data_len = sizeof(adv_mfg_config),
		 .data = (uint8_t *)&adv_mfg_config}

	};

	// manuall adv param PDU
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
		// 0x09
		BT_DATA(BT_DATA_URI, Ble_structs::mygithub, sizeof(Ble_structs::mygithub)),
		// nordic  led service uuid foud in uuid.h
		// BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		// BT_UUID_128_ENCODE(0x00001525,
		//  0x1212, 0xefde, 0x1523, 0x785feabcd123)),
		//

	};

	static void button_changed()
	{
		if (Ble_structs::adv_mfg_config.custom_data < 0xfffe) {
			bt_le_adv_update_data(Ble_structs::ad, ARRAY_SIZE(Ble_structs::ad),
					      Ble_structs::scan_response,
					      ARRAY_SIZE(Ble_structs::scan_response));
			if (Ble_structs::adv_mfg_config.custom_data == 0xfffe) {
				Ble_structs::adv_mfg_config.custom_data = 0;
			}
		}
	}

	static inline  bt_addr_le_t addr;
	static  inline int random_id = BT_ID_DEFAULT;

	static int Randomize_address()
	{
		int err = bt_addr_le_from_str("D2:44:33:22:11:00", "random-id", &Ble_structs::addr);
		if (err) {
			return -EINVAL;
		}
		err = bt_id_create(&Ble_structs::addr, NULL); //address to put on ,
		// the other para is the IRK handled by l2adp layer
		if (err < 0) {
			return err;
		}
		random_id=  err;
		Ble_structs::adv_param.id = Ble_structs::random_id;	


		return 0;
	}


// //resume the advertisation  after   central disconnection as ISR
// static void   adv_work_handler(struct k_work *work){
// int err= bt_le_adv_start(&Ble_structs::adv_param,
// 		  Ble_structs::ad, ARRAY_SIZE(Ble_structs::ad),
// 		  Ble_structs::scan_response,
// 		  ARRAY_SIZE(Ble_structs::scan_response));
// 		  if(err) {
// 			printk("Advertising failerd to start (err %d\n)",  err);
// 			return -1;
// 		  }else {
// 			printk("Advertising started successfully\n");
// 		  }
// }

// static void advertising_start(void)
// {

// k_work_submit(&adv_work_handler);
// }


// static void recycled_work(void){
// 	printk("Connection object available from previous conn. Disconnect is complete!\n");
// 	advertising_start();
// }
// BT_CONN_CB_DEFINE(conn_callbacks) = {
// 	.recycled = recycled_work,				
// };
};

#endif // BT  || Perpherial
#endif // BLE_STRUCTS_HPP
