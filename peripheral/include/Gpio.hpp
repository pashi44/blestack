#ifndef GPIO_HPP
#define GPIO_HPP


#if  defined (CONFIG_GPIO) 
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

#if   defined(CONFIG_GPIO)
        static inline const struct gpio_dt_spec led_blue = 
            GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

        static int gpio_init() {
            if (!device_is_ready(led_blue.port)) {
                return -ENODEV;
            }
            return gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
        }

        static int gpio_pulse(uint16_t duration_ms) {
            int ret = gpio_pin_set_dt(&led_blue, 1);
            if (ret < 0) return ret;
            k_msleep(duration_ms);
            return gpio_pin_set_dt(&led_blue, 0);
        }
#else
        static int gpio_init() { return 0; }
        static int gpio_pulse(uint16_t duration_ms) { return 0; }
#endif //CONFIG_GPIO
 



//pwm -rgb section 
#if defined(CONFIG_PWM) && defined(CONFIG_GPIO)    


static inline  const struct pwm_dt_spec red_pwm_led = 
	PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
static inline  const struct pwm_dt_spec green_pwm_led = 
	PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));
static inline  const struct pwm_dt_spec blue_pwm_led = 
	PWM_DT_SPEC_GET(DT_ALIAS(pwm_led2));
#endif








};



}

#endif // GPIO_HPP