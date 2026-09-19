#pragma once
// Hardware-independent declarations; the selected backend supplies definitions.
namespace Gpios {

extern void configure_leds(void);
extern   void button_init(void);

} // namespace Gpio