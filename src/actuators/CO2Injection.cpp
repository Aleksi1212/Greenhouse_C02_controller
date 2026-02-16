//
// Created by Mmiud on 2/12/2026.
//

#include "CO2Injection.h"
#include "hardware/gpio.h"
#include <stdio.h>

CO2Injection::CO2Injection(int pinNr) : GPIOPin(pinNr)
{
    gpio_init(GPIOPin);
    gpio_set_dir(GPIOPin, GPIO_OUT);
}

void CO2Injection::set(uint16_t value) {
    if (value == 1) printf("Opening valve\n");
    else printf("closing valve\n");
    gpio_put(GPIOPin, value);
}

bool CO2Injection::getStatus() {
    return gpio_get(GPIOPin);
}


