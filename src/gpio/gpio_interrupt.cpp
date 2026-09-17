

#include "gpio/gpio_interrupt.hpp"
#include "gpio/nrf9151_registers.hpp"

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <cstdint>


LOG_MODULE_REGISTER(gpio_interrupt, LOG_LEVEL_DBG);
    
    // This backend owns the GPIOTE interrupt and assumes nRF9151 DK wiring.
    #if !defined(CONFIG_BOARD_NRF9151DK)
    #error "Raw GPIO backend requires the nRF9151 DK"
    #endif
    #if defined(CONFIG_NRFX_GPIOTE)
#error "Disable competing nrfx GPIOTE interrupt ownership for this raw backend"
#endif

// Raw nRF9151 DK wiring: GPIO0, active-high LEDs, active-low button.
// These constants directly select pins in the memory-mapped GPIO Registers::Gpio.
constexpr uint32_t LED0_PIN = 0U;
constexpr uint32_t LED1_PIN = 1U;
constexpr uint32_t LED2_PIN = 4U;
constexpr uint32_t LED3_PIN = 5U;
constexpr uint32_t BUTTON0_PIN = 8U;

// Optional devicetree alternatives
// #include <zephyr/devicetree/gpio.h>
// constexpr uint32_t LED0_PIN = DT_GPIO_PIN(DT_ALIAS(flickled0), gpios);
// constexpr uint32_t LED1_PIN = DT_GPIO_PIN(DT_ALIAS(flickled1), gpios);
// constexpr uint32_t LED2_PIN = DT_GPIO_PIN(DT_ALIAS(flickled2), gpios);
// constexpr uint32_t LED3_PIN = DT_GPIO_PIN(DT_ALIAS(flickled3), gpios);
// constexpr uint32_t BUTTON0_PIN = DT_GPIO_PIN(DT_ALIAS(sw0), gpios);

static volatile uint32_t * const p0_input =
    reinterpret_cast<volatile uint32_t *>(Registers::Gpio::p0_base + 0x010UL);
static volatile uint32_t * const button_pin_cnf =
    reinterpret_cast<volatile uint32_t *>(
        Registers::Gpio::p0_base + Registers::Gpio::pin_cnf + BUTTON0_PIN * sizeof(uint32_t));

static volatile uint32_t &p0_outset = Registers::at(Registers::Gpio::p0_base + Registers::Gpio::outset);
static volatile uint32_t &p0_outclr = Registers::at(Registers::Gpio::p0_base + Registers::Gpio::outclr);
static volatile uint32_t &p0_dirset = Registers::at(Registers::Gpio::p0_base + Registers::Gpio::dirset);
static volatile uint32_t &gpiote_events_port = Registers::at(Registers::Gpio::gpiote_base + Registers::Gpio::events_port);
static volatile uint32_t &gpiote_intenset = Registers::at(Registers::Gpio::gpiote_base + Registers::Gpio::intenset);
static volatile uint32_t &gpiote_intenclr = Registers::at(Registers::Gpio::gpiote_base + Registers::Gpio::intenclr);

// NVIC ICPR starts at 0xE000E280; each 32-bit register covers 32 IRQs.
static volatile uint32_t * const nvic_pending_clear =
    reinterpret_cast<volatile uint32_t *>(
        0xE000E280UL + (Registers::Gpio::gpiote_irq / 32U) * sizeof(uint32_t));
constexpr uint32_t gpiote_nvic_mask = 1UL << (Registers::Gpio::gpiote_irq % 32U);

constexpr uint32_t LED_PIN_MASK = (1U << LED0_PIN) | (1U << LED3_PIN) ;
                                //    (1U << LED2_PIN) | (1U << LED3_PIN);

constexpr uint32_t LED_ON_DURATION_MS = 1000U;

static void button_isr(const void *arg);
static void led_off_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(led_off_work, led_off_work_handler);

void Gpios::configure_leds(void)
{
    constexpr uint32_t cnf_feature_mask = 0x00000003UL; /* DIR=Output, INPUT=Disconnect */

    // Preload all four active-high LEDs low before making their pins outputs.
    p0_outclr = (1U << LED0_PIN) | (1U << LED1_PIN) |
                (1U << LED2_PIN) | (1U << LED3_PIN);

    volatile uint32_t *led_cnf_Registers[] = {
        reinterpret_cast<volatile uint32_t *>(Registers::Gpio::p0_base + Registers::Gpio::pin_cnf + (LED0_PIN * 4U)),
        reinterpret_cast<volatile uint32_t *>(Registers::Gpio::p0_base + Registers::Gpio::pin_cnf + (LED1_PIN * 4U)),
        reinterpret_cast<volatile uint32_t *>(Registers::Gpio::p0_base + Registers::Gpio::pin_cnf + (LED2_PIN * 4U)),
        reinterpret_cast<volatile uint32_t *>(Registers::Gpio::p0_base + Registers::Gpio::pin_cnf + (LED3_PIN * 4U)),
    };

    for (volatile uint32_t *cnf_reg : led_cnf_Registers) {
        *cnf_reg = cnf_feature_mask;
    }

    p0_dirset = LED_PIN_MASK;
}

void  lit_the_leds(void)
{
    p0_outset = LED_PIN_MASK;
}

 void led_off_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    p0_outclr = LED_PIN_MASK;
    LOG_INF("LEDs off");
}

 void button_isr(const void *arg)
{
    ARG_UNUSED(arg);

    if (gpiote_events_port == 0U) {
        return;
    }

    gpiote_events_port = 0;
    const uint32_t cleared_event = gpiote_events_port; // Complete the MMIO readback.
    (void)cleared_event;

    if ((*p0_input & (1UL << BUTTON0_PIN)) != 0U) {
        return;
    }

    lit_the_leds();
    LOG_INF("Button pressed, lighting LEDs for %u ms", LED_ON_DURATION_MS);
    k_work_schedule(&led_off_work,  K_MSEC(LED_ON_DURATION_MS)); 
}

void Gpios::button_init(void)
{
    irq_disable(Registers::Gpio::gpiote_irq);

    gpiote_intenclr = Registers::Gpio::port_interrupt_mask;
    const uint32_t disabled_interrupts = gpiote_intenclr;
    (void)disabled_interrupts; 


    constexpr uint32_t button_pullup = 3UL << 2;
    *button_pin_cnf = button_pullup; // Input connected, pull-up, SENSE disabled.
    k_busy_wait(10); // Let the pull-up settle; this is not button debounce.
    const bool initially_released = (*p0_input & (1UL << BUTTON0_PIN)) != 0U;
    LOG_DBG("Button input before SENSE: %s", initially_released ? "high" : "low");
    *button_pin_cnf = button_pullup | (3UL << 16); // SENSE=Low.

    gpiote_events_port = 0;           
    const uint32_t cleared_event = gpiote_events_port;
    (void)cleared_event; // Complete the event clear before clearing NVIC.
    *nvic_pending_clear = gpiote_nvic_mask;

    IRQ_CONNECT(Registers::Gpio::gpiote_irq, 4, button_isr, nullptr, 0); //3,6 & 7 are soft priority uses in ble applications


    gpiote_intenset = Registers::Gpio::port_interrupt_mask; //Re-enable PORT interrupt 
     irq_enable(Registers::Gpio::gpiote_irq)  ; 
 /*
 
 *reinterpret_cast<volatile uint32_t *>(
    Registers::Gpio::nvic_base + 0x004UL 
 ) |= (1UL << 17);   //  jump to non-secure  intrr setregister and mask at 17 th buit
 //sets  the iset but it remais to toggle  can be cleared but didnt find the Iclr regisetr offset; lest stick to the apis for now 
*/

}

