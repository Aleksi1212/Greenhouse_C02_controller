//
// Created by renek on 07/03/2026.
//

#ifndef ROTARYENCODER_H
#define ROTARYENCODER_H

#include "FreeRTOS.h"
#include "task.h"

class RotaryEncoder {
public:
    RotaryEncoder(uint clkPin, uint dtPin, uint btnPin, uint32_t debounceMs = 300);

    // this returns accumulated ticks since last call and resets to 0 v
    int  getDelta();
    bool wasButtonPressed();

private:
    uint rot_a;
    uint rot_b;
    uint rotBtn;
    uint32_t debounceMs;

    TickType_t lastButtonTick{0};

    volatile int  encoderValue{0};
    volatile bool buttonPressed{false};

    static RotaryEncoder* instance;
    static void gpioCallback(uint gpio, uint32_t events);
};

#endif // ROTARYENCODER_H
