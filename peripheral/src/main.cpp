#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>


#define DEVICE_NAME  CONFIG_BT_DEVICE_NAME

LOG_MODULE_REGISTER(Ble_sample_main, LOG_LEVEL_DBG);



static const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
#if defined(CONFIG_BT)
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h> //type of connection
// #include <zephyr/bluetooth/uuid.h>
#endif

//manuall adv param PDU

static const struct bt_le_adv_param adv_param =


{

	.id=  NULL,
	.sid = NULL,	//advertising set identifier
	.secondary_max_skip = NULL,

};



static const uint8_t ad_flags = BT_LE_AD_NO_BREDR;
static const struct bt_data ad[] = {

{

	.type = (uint8_t)BT_DATA_BROADCAST_NAME,
        .data_len = (uint8_t)1,
        .data = &ad_flags,
    },

    // BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_NO_BREDR)),

{
	.type = BT_DATA_NAME_COMPLETE,
	.data_len = sizeof(DEVICE_NAME) - 1,
	.data = (uint8_t *)DEVICE_NAME
}
	
    // BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, sizeof(DEVICE_NAME) - 1)
};

 static unsigned char mygithub[] = {0x17, '/', '/', 'g', 'i', 't', 'h', 'u', 'b', '.', 'c',
				   'o',  'm', '/', 'p', 'a', 's', 'h', 'i', '4', '4'};

// scan response PDUS

static const struct bt_data scan_response[] = { //0x09
	BT_DATA(BT_DATA_URI, mygithub, sizeof(mygithub))};

#ifdef __cplusplus__
extern "C" {
#endif

int main(void)
{


if (!gpio_is_ready_dt(&led_blue)) {
        return  -ENODEV;
    }
    gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_ACTIVE);
	if (!gpio_is_ready_dt(&led_blue)) {
        return -ENODEV;
    }else
    gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_ACTIVE);

	int err = bt_enable(NULL);

	if (err) {
		LOG_ERR("Bluetooth initialization failed\n");
		return -ENODEV;
	} else {
		LOG_INF("Ble initalized\n");
	}

	// starting le advert
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
     scan_response,
			      ARRAY_SIZE(scan_response));

	if (err) {
		LOG_ERR("Bluetooth advertising failed %d\t\n", err);
		return -ENODEV;
	} else {
		LOG_INF("Ble advertising started\n");
	}

	while (1) {
		LOG_INF("spitting he serial signals fo rble\n");

		k_sleep(K_MSEC(1000));
	}

	return 0;
}

#ifdef __cplusplus__
}

#endif // DEBUG
