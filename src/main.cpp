#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(led_fix, LOG_LEVEL_DBG);

/* Extract exact Devicetree pin numbers */
constexpr uint32_t LED0_PIN = DT_GPIO_PIN(DT_ALIAS(led0), gpios);
constexpr uint32_t LED1_PIN = DT_GPIO_PIN(DT_ALIAS(led1), gpios);
constexpr uint32_t LED2_PIN = DT_GPIO_PIN(DT_ALIAS(led2), gpios);
constexpr uint32_t LED3_PIN = DT_GPIO_PIN(DT_ALIAS(led3), gpios);

constexpr uint32_t ALL_LEDS_MASK = (1 << LED0_PIN) | 
                                    (1 << LED1_PIN) | 
                                    (1 << LED2_PIN) | 
                                    (1 << LED3_PIN);

/* Use 0x50842500UL for Secure targets, 0x40842500UL for Non-Secure (NS) targets */
#if defined(CONFIG_TRUSTED_EXECUTION_SECURE) || !defined(CONFIG_BUILD_WITH_TFM)
  #define NRF_P0_BASE 0x50842500UL  /* Secure GPIO Base */
#else
  #define NRF_P0_BASE 0x40842500UL  /* Non-Secure GPIO Base */
#endif

/* Register References */
volatile uint32_t &p0_out    = *reinterpret_cast<volatile uint32_t*>(NRF_P0_BASE + 0x004UL);
volatile uint32_t &p0_outset = *reinterpret_cast<volatile uint32_t*>(NRF_P0_BASE + 0x008UL);
volatile uint32_t &p0_outclr = *reinterpret_cast<volatile uint32_t*>(NRF_P0_BASE + 0x00CUL);
volatile uint32_t &p0_dirset = *reinterpret_cast<volatile uint32_t*>(NRF_P0_BASE + 0x018UL);

/* PIN_CNF References */
volatile uint32_t &p0_pin_cnf_led0 = *reinterpret_cast<volatile uint32_t*>(NRF_P0_BASE + 0x200UL + (LED0_PIN * 4));
volatile uint32_t &p0_pin_cnf_led1 = *reinterpret_cast<volatile uint32_t*>(NRF_P0_BASE + 0x200UL + (LED1_PIN * 4));
volatile uint32_t &p0_pin_cnf_led2 = *reinterpret_cast<volatile uint32_t*>(NRF_P0_BASE + 0x200UL + (LED2_PIN * 4));
volatile uint32_t &p0_pin_cnf_led3 = *reinterpret_cast<volatile uint32_t*>(NRF_P0_BASE + 0x200UL + (LED3_PIN * 4));

#ifdef __cplusplus
extern "C" {
#endif

int main(void)
{
    /* DIR = Output (bit 0 = 1), INPUT = Disconnect (bit 1 = 1) */
    constexpr uint32_t cnf_output_val = (1 << 0) | (1 << 1);

    p0_pin_cnf_led0 = cnf_output_val;
    p0_pin_cnf_led1 = cnf_output_val;
    p0_pin_cnf_led2 = cnf_output_val;
    p0_pin_cnf_led3 = cnf_output_val;

    /* Set direction as output */
    p0_dirset = ALL_LEDS_MASK;

    /* 1. Drive pins LOW -> Turn LEDs ON for 1 second */
    p0_outclr = ALL_LEDS_MASK;
    k_msleep(1000);

    /* 2. Force pins HIGH via direct OUT register write -> Turn LEDs OFF */
    p0_out |= ALL_LEDS_MASK; 

    /* 3. Infinite hold loop */
    while (1) {
        k_msleep(1000);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif