#ifndef GPIO_HPP
#define GPIO_HPP

#if defined(CONFIG_GPIO)
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#endif

#if defined(CONFIG_PWM) && defined(CONFIG_GPIO)
#include <zephyr/drivers/pwm.h>
#endif
#include <cstdint>

namespace GPIO
{
struct Gpio {
	Gpio() = delete;
	~Gpio() = delete;

#if defined(CONFIG_GPIO) && defined(CONFIG_PWM) 

	static inline const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
	static inline const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
	static inline const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

	// gpio  port  init
	static int gpio_init()
	{
		if (!device_is_ready(led_blue.port) || (!device_is_ready(led_red.port)) ||
		    (!device_is_ready(led_green.port))) {
			return -ENODEV;
		}
		int ret;
		ret = gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			return ret;
		}

		ret = gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			return ret;
		}

		ret = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
		return ret;
	}	
    //gpio _pulse defination
	static int gpio_pulse(const struct gpio_dt_spec *led, uint16_t duration_ms)
	{
		int ret = gpio_pin_set_dt(led, 1);
		if (ret < 0) {
			return ret;
		}
		k_msleep(duration_ms);
		return gpio_pin_set_dt(led, 0);
	}

static int pulse_all(uint16_t duration_ms) {
    gpio_pin_set_dt(&GPIO::Gpio::led_red, 1);
    gpio_pin_set_dt(&GPIO::Gpio::led_green, 1);
    gpio_pin_set_dt(&GPIO::Gpio::led_blue, 1);

    k_msleep(duration_ms);

    gpio_pin_set_dt(&GPIO::Gpio::led_red, 0);
    gpio_pin_set_dt(&GPIO::Gpio::led_green, 0);
    return gpio_pin_set_dt(&GPIO::Gpio::led_blue, 0);
}
#else
	static int gpio_init()
	{
		return 0;
	}
	static int gpio_pulse(const struct gpio_dt_spec *led, uint16_t duration_ms)
	{
		return 0;
	}
#endif // CONFIG_GPIO

// pwm -rgb section
#if defined(CONFIG_PWM) && defined(CONFIG_GPIO)

	static inline const struct pwm_dt_spec red_pwm_led =
		PWM_DT_SPEC_GET(DT_NODELABEL(red_pwm_led));
	static inline const struct pwm_dt_spec green_pwm_led =
		PWM_DT_SPEC_GET(DT_NODELABEL(green_pwm_led));
	static inline const struct pwm_dt_spec blue_pwm_led =
		PWM_DT_SPEC_GET(DT_NODELABEL(blue_pwm_led));
#endif
};

} // namespace GPIO

#endif // GPIO_HPP
