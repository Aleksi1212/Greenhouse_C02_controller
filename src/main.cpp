#include <iostream>
#include <sstream>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "ssd1306.h"
#include "SensorInterface.h"
#include "ModbusClient.h"
#include <array>
#include <vector>

#include "CO2Injection.h"
#include "CO2Sensor.h"
#include "ProdualMIO.h"
#include "RhSensor.h"
#include "TempSensor.h"
#include "thingspeak.h"
#include "controller/CO2Controller.h"
#include "hardware/timer.h"
extern "C" {
    uint32_t read_runtime_ctr(void) {
        return timer_hw->timerawl;
    }
}

#include "blinker.h"

#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define BAUD_RATE 9600
#define STOP_BITS 2 // for real system (pico simualtor also requires 2 stop bits)




int main() {

    // static auto uart{std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE, STOP_BITS)};
    // static auto rtu_client{std::make_shared<ModbusClient>(uart)};

    // static std::array<std::shared_ptr<SensorInterface>, 3> sensors = {
    //     std::make_shared<CO2Sensor>(rtu_client, 240, 256, 2049),
    //     std::make_shared<TempSensor>(rtu_client, 241, 257, 512),
    //     std::make_shared<RhSensor>(rtu_client, 241, 256, 512)
    // };

    // static std::array<std::shared_ptr<ActuatorsInterface>, 2> actuators = {
    //     std::make_shared<ProdualMIO>(rtu_client, 1, 0, 4),
    //     std::make_shared<CO2Injection>(27)
    // };

    // static CO2Controller controller(sensors, actuators);
    ThingSpeak ts;

    stdio_init_all();
    printf("\nBoot\n");

    vTaskStartScheduler();
    while (1) {printf("hello\n");};
}