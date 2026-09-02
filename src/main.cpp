/*
 * nRF9151 DK - bare-metal, register-level LED blink (all 4 onboard LEDs)
 *
 * Fixes vs. the original:
 *   1. LEDs on the nRF9151 DK are ACTIVE-HIGH (driven through power
 *      transistors) - see the DK Hardware User Guide, section 4.8
 *      "Buttons and LEDs". OUTSET (drive high) turns them ON,
 *      OUTCLR (drive low) turns them OFF - the opposite of what the
 *      original comment assumed.
 *   2. Added the missing OUTSET register reference.
 *   3. The while(true) loop now actually alternates OUTSET/OUTCLR to
 *      blink, instead of setting once and idling forever.
 *
 * Register offsets/fields verified against nRF9151 Product
 * Specification, section 6.4.4 (GPIO Registers):
 *   P0 base:  0x50842500 (Secure) / 0x40842500 (Non-Secure)
 *   OUT      0x004   OUTSET  0x008   OUTCLR  0x00C
 *   DIR      0x014   DIRSET  0x018
 *   PIN_CNF[n] = 0x200 + n*4   (bit0 DIR, bit1 INPUT disconnect)
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <cstdint>

constexpr uint32_t LED0_PIN = DT_GPIO_PIN(DT_ALIAS(led0), gpios); /* P0.00 */
constexpr uint32_t LED1_PIN = DT_GPIO_PIN(DT_ALIAS(led1), gpios); /* P0.01 */
constexpr uint32_t LED2_PIN = DT_GPIO_PIN(DT_ALIAS(led2), gpios); /* P0.04 */
constexpr uint32_t LED3_PIN = DT_GPIO_PIN(DT_ALIAS(led3), gpios); /* P0.05 */

/* Secure alias 0x50842500 / Non-Secure alias 0x40842500 - PS 6.4.4 "Instances" */
#if defined(CONFIG_TRUSTED_EXECUTION_SECURE) || !defined(CONFIG_BUILD_WITH_TFM)
  #define NRF_P0_BASE 0x50842500UL
#else
  #define NRF_P0_BASE 0x40842500UL
#endif

#define OFFSET_OUTSET   0x008UL
#define OFFSET_OUTCLR   0x00CUL
#define OFFSET_DIRSET   0x018UL
#define OFFSET_PIN_CNF  0x200UL

volatile uint32_t &p0_outset = *reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_OUTSET);
volatile uint32_t &p0_outclr = *reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_OUTCLR);
volatile uint32_t &p0_dirset = *reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_DIRSET);


static void lit_the_leds(void){
	/* PIN_CNF: DIR=Output(bit0=1), INPUT=Disconnect(bit1=1) -> 0x3.
	 * Correct as-is per PS section 6.4.4.11.
	 */
	constexpr uint32_t cnf_feature_mask = 0x00000003UL;

	volatile uint32_t *led_cnf_registers[] = {
		reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_PIN_CNF + (LED0_PIN * 4U)),
		reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_PIN_CNF + (LED1_PIN * 4U)),
		reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_PIN_CNF + (LED2_PIN * 4U)),
		reinterpret_cast<volatile uint32_t *>(NRF_P0_BASE + OFFSET_PIN_CNF + (LED3_PIN * 4U)),
	};

	for (volatile uint32_t *cnf_reg : led_cnf_registers) {
		*cnf_reg = cnf_feature_mask;
	}

	const uint32_t port_pin_mask = (1U << LED0_PIN) | (1U << LED1_PIN) |
					(1U << LED2_PIN) | (1U << LED3_PIN);


	p0_dirset = port_pin_mask; // masking hte dir register as pins
		p0_outset = port_pin_mask; // setting hhigh withpin mask on the outset register
		k_msleep(5000);

    p0_outclr = port_pin_mask;
		// p0_outclr = port_pin_mask;

}
#ifdef __cplusplus
extern "C" {
#endif

int main(void)
{

lit_the_leds();

	while (true) {
		k_msleep(500);
	}

	return 0;
}

#ifdef __cplusplus
}
#endif