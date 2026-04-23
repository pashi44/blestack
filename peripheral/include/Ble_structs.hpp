#ifndef  __BLE_STRUCTS_HPP__
#define  __BLE_STRUCTS_HPP__


#ifdef BT
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>  
#include <zephyr/bluetooth/assigned_numbers.h>

#endif // DEBUG

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define COMPANY_ID_CODE ( 0x0059)



struct Ble_structs{
static inline  const uint8_t ad_flags = BT_LE_AD_NO_BREDR;


    Ble_structs() = delete; 
   Ble_structs(const Ble_structs &) = delete;
   Ble_structs &operator=(const Ble_structs &) = delete;    

struct adv_mfg_config {
    uint16_t company_id;
    uint16_t custom_data;
};

static inline  const struct Ble_structs::adv_mfg_config adv_mfg_config = {
     COMPANY_ID_CODE,  0x00
};



static  inline  const struct bt_data ad[] = {
	{

		.type = (uint8_t)  BT_DATA_FLAGS,
		.data_len = (uint8_t)sizeof(ad_flags),
		.data = &ad_flags,
	},

	// BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_NO_BREDR)),

	{.type = (uint8_t) BT_DATA_NAME_COMPLETE,
	 .data_len = sizeof(DEVICE_NAME) - 1,
	 .data = (uint8_t *)DEVICE_NAME,
	},

{
	.type = (uint8_t) BT_DATA_MANUFACTURER_DATA,  //0xff
	.data_len = sizeof(adv_mfg_config),
	.data = (uint8_t *)&adv_mfg_config
}


};



// manuall adv param PDU
static inline const struct bt_le_adv_param adv_param = {
    .id = BT_ID_DEFAULT,
    .sid = 0,
	.secondary_max_skip = 0,
    .options = BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_EXT_ADV,
    .interval_min = 80,
    .interval_max = 160,
    .peer = NULL,
};


static inline unsigned char mygithub[] = {
    0x17, '/', '/', 'g', 'i', 't', 'h', 'u', 'b', '.', 'c',
 'o','m', '/', 'p', 'a', 's', 'h', 'i', '4', '4'
};

// static void button_changed(uint32_t button_state, uint32_t has_changed)
// {
//     if (has_changed & button_state & USER_BUTTON) {
//         adv_mfg_data.number_press += 1;
//         bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
//     }
// }

// scan response PDUS

static inline const struct bt_data scan_response[] = { // 0x09
	BT_DATA(BT_DATA_URI, Ble_structs::mygithub, sizeof(Ble_structs::mygithub))};


};




#endif // ! __BLE_STRUCTS_HPP__
