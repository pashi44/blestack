#include "gpio/gpio_interrupt.hpp"
#include "gpio/nrf9151_registers.hpp"

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <cstdint>

namespace gpio {

// This backend owns the GPIOTE interrupt and assumes nRF9151 DK wiring.
#if !defined(CONFIG_BOARD_NRF9151DK)
#error "Raw GPIO backend requires the nRF9151 DK"
#endif
#if defined(CONFIG_NRFX_GPIOTE)
#error "Disable competing nrfx GPIOTE interrupt ownership for this raw backend"
#endif

// DK board wiring (not defined by the chip's register map): LEDs active-high,
// Button 1 active-low. Devicetree overlays do not change these pin assignments.
constexpr uint32_t LED0_PIN = 0U;
constexpr uint32_t LED1_PIN = 1U;
constexpr uint32_t LED2_PIN = 4U;
constexpr uint32_t LED3_PIN = 5U;
constexpr uint32_t BUTTON0_PIN = 8U;

static volatile uint32_t &p0_outset = registers::at(registers::p0_base + registers::outset);
static volatile uint32_t &p0_outclr = registers::at(registers::p0_base + registers::outclr);
static volatile uint32_t &p0_dirset = registers::at(registers::p0_base + registers::dirset);
static volatile uint32_t &gpiote_events_port = registers::at(registers::gpiote_base + registers::events_port);
static volatile uint32_t &gpiote_intenset = registers::at(registers::gpiote_base + registers::intenset);
static volatile uint32_t &gpiote_intenclr = registers::at(registers::gpiote_base + registers::intenclr);

constexpr uint32_t LED_PIN_MASK = (1U << LED0_PIN) | (1U << LED1_PIN) ;
                                //    (1U << LED2_PIN) | (1U << LED3_PIN);

constexpr uint32_t LED_ON_DURATION_MS = 10000U;

static void button_isr(const void *arg);
static void led_off_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(led_off_work, led_off_work_handler);

void configure_leds(void)
{
    constexpr uint32_t cnf_feature_mask = 0x00000003UL; /* DIR=Output, INPUT=Disconnect */

    volatile uint32_t *led_cnf_registers[] = {
        reinterpret_cast<volatile uint32_t *>(registers::p0_base + registers::pin_cnf + (LED0_PIN * 4U)),
        reinterpret_cast<volatile uint32_t *>(registers::p0_base + registers::pin_cnf + (LED1_PIN * 4U)),
        reinterpret_cast<volatile uint32_t *>(registers::p0_base + registers::pin_cnf + (LED2_PIN * 4U)),
        reinterpret_cast<volatile uint32_t *>(registers::p0_base + registers::pin_cnf + (LED3_PIN * 4U)),
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
    const uint32_t cleared_event = gpiote_events_port; // Complete the MMIO readback.
    (void)cleared_event;
    lit_the_leds();
    k_work_schedule(&led_off_work,  K_MSEC(LED_ON_DURATION_MS)); //offloadingt the GPIO off
}

void button_init(void)
{
    // Mask PORT before enabling SENSE, as required by section 6.5.2.
    gpiote_intenclr = registers::port_interrupt_mask;
    *reinterpret_cast<volatile uint32_t *>(registers::p0_base + registers::pin_cnf + (BUTTON0_PIN * 4U)) =
        (3UL << 2) | (3UL << 16); /* PULL=Pullup, SENSE=Low (active-low button) */

    gpiote_events_port = 0;           /* 2. Clear any pending event */

    // Zephyr still installs the ISR and enables the NVIC line. IRQ numbers
    // come from the datasheet; no devicetree/device binding is used here.
    IRQ_CONNECT(registers::gpiote_irq, 4, button_isr, nullptr, 0);
    irq_enable(registers::gpiote_irq);

    gpiote_intenset = registers::port_interrupt_mask; /* 4. Re-enable PORT interrupt */
}

} // namespace gpio
