#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <cstdint>

constexpr uint32_t LED0_PIN    = DT_GPIO_PIN(DT_ALIAS(led0), gpios); /* P0.00 */
constexpr uint32_t LED1_PIN    = DT_GPIO_PIN(DT_ALIAS(led1), gpios); /* P0.01 */
constexpr uint32_t LED2_PIN    = DT_GPIO_PIN(DT_ALIAS(led2), gpios); /* P0.04 */
constexpr uint32_t LED3_PIN    = DT_GPIO_PIN(DT_ALIAS(led3), gpios); /* P0.05 */
constexpr uint32_t BUTTON0_PIN = DT_GPIO_PIN(DT_ALIAS(sw0), gpios);  /* P0.08 */

#if defined(CONFIG_TRUSTED_EXECUTION_SECURE) || !defined(CONFIG_BUILD_WITH_TFM)
  #define NRF_P0_BASE      0x50842500UL
  #define NRF_GPIOTE_BASE  0x50031000UL
#else
  #define NRF_P0_BASE      0x40842500UL
  #define NRF_GPIOTE_BASE  0x40031000UL
#endif

#define OFFSET_OUTSET   0x008UL
#define OFFSET_OUTCLR   0x00CUL
#define OFFSET_DIRSET   0x018UL
#define OFFSET_PIN_CNF  0x200UL

volatile uint32_t &p0_outset = *reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_OUTSET);
volatile uint32_t &p0_outclr = *reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_OUTCLR);
volatile uint32_t &p0_dirset = *reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_DIRSET);

volatile uint32_t &gpiote_events_port = *reinterpret_cast<volatile uint32_t *>(NRF_GPIOTE_BASE + 0x17CUL);
volatile uint32_t &gpiote_intenset    = *reinterpret_cast<volatile uint32_t *>(NRF_GPIOTE_BASE + 0x304UL);
volatile uint32_t &gpiote_intenclr    = *reinterpret_cast<volatile uint32_t *>(NRF_GPIOTE_BASE + 0x308UL);

constexpr uint32_t LED_PIN_MASK = (1U << LED0_PIN) | (1U << LED1_PIN) |
                                   (1U << LED2_PIN) | (1U << LED3_PIN);

constexpr uint32_t LED_ON_DURATION_MS = 10000U;

static void button_isr(const void *arg);
static void led_off_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(led_off_work, led_off_work_handler);

static void leds_configure_as_output(void)
{
    constexpr uint32_t cnf_feature_mask = 0x00000003UL; /* DIR=Output, INPUT=Disconnect */

    volatile uint32_t *led_cnf_registers[] = {
        reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_PIN_CNF + (LED0_PIN * 4U)),
        reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_PIN_CNF + (LED1_PIN * 4U)),
        reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_PIN_CNF + (LED2_PIN * 4U)),
        reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_PIN_CNF + (LED3_PIN * 4U)),
    };

    for (volatile uint32_t *cnf_reg : led_cnf_registers) {
        *cnf_reg = cnf_feature_mask;
    }

    p0_dirset = LED_PIN_MASK;
}

static void lit_the_leds(void)
{
    p0_outset = LED_PIN_MASK;
}

static void led_off_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    p0_outclr = LED_PIN_MASK;
}

static void button_isr(const void *arg)
{
    ARG_UNUSED(arg);
    gpiote_events_port = 0;   /* Clear the GPIOTE port event */
    lit_the_leds();
    k_work_schedule(&led_off_work,  K_MSEC(LED_ON_DURATION_MS)); //offloadingt the GPIO off
}

void button_init(void)
{
    *reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_PIN_CNF + (BUTTON0_PIN * 4U)) =
        (3UL << 2) | (3UL << 16); /* PULL=Pullup, SENSE=Low (active-low button) */

    gpiote_intenclr    = (1UL << 31); /* 1. Disable PORT interrupt while configuring */
    gpiote_events_port = 0;           /* 2. Clear any pending event */

    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(gpiote)), 4, button_isr, nullptr, 0);
    irq_enable(DT_IRQN(DT_NODELABEL(gpiote)));

    gpiote_intenset = (1UL << 31); /* 4. Re-enable PORT interrupt */
}

#ifdef __cplusplus
extern "C" {
#endif

int main(void)
{
    leds_configure_as_output(); /* one-time pin setup, run once at boot */
    button_init();

    while (true) {
        k_msleep(1000);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif