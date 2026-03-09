//
// Created by renek on 07/03/2026.
//

#include "RotaryEncoder.h"
#include "hardware/gpio.h"

RotaryEncoder* RotaryEncoder::instance{nullptr};

RotaryEncoder::RotaryEncoder(uint clkPin, uint dtPin, uint btnPin, uint32_t debounceMs)
    : rot_a(clkPin), rot_b(dtPin), rotBtn(btnPin), debounceMs(debounceMs)
{
    instance = this;

    gpio_init(clkPin);
    gpio_set_dir(clkPin, GPIO_IN);
    gpio_pull_up(clkPin);

    gpio_init(dtPin);
    gpio_set_dir(dtPin, GPIO_IN);
    gpio_pull_up(dtPin);

    gpio_init(btnPin);
    gpio_set_dir(btnPin, GPIO_IN);
    gpio_pull_up(btnPin);

    gpio_set_irq_enabled_with_callback(clkPin, GPIO_IRQ_EDGE_FALL, true, &RotaryEncoder::gpioCallback);
    gpio_set_irq_enabled(btnPin, GPIO_IRQ_EDGE_FALL, true);
}

int RotaryEncoder::getDelta() {

    const int delta = encoderValue;
    encoderValue = 0;
    return delta;
}

bool RotaryEncoder::wasButtonPressed() {

    if (!buttonPressed) return false;
    buttonPressed = false;
    TickType_t now = xTaskGetTickCount();

    if ((now - lastButtonTick) >= pdMS_TO_TICKS(debounceMs)) {
        lastButtonTick = now;
        return true;
    }
    return false;
}

void RotaryEncoder::gpioCallback(uint gpio, uint32_t events) {
    if (gpio == instance->rot_a) {

        if (gpio_get(instance->rot_b)) {
            instance->encoderValue = instance->encoderValue + 1;
        } else {
            instance->encoderValue = instance->encoderValue - 1;
        }

    } else if (gpio == instance->rotBtn) {
        instance->buttonPressed = true;
    }
}
