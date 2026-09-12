#pragma once
    #include <zephyr/logging/log.h>

#include "nrf9151_registers.hpp"



namespace gpio {

extern void configure_leds(void);
extern void button_init(void);


namespace gpiote{

//nivc registermap


} //namespace giote
} // namespace gpio
