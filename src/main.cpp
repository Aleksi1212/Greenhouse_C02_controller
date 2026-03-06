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
#include "Fmutex.h"
#include <array>
#include <vector>

#include "CO2Injection.h"
#include "CO2Sensor.h"
#include "ProdualMIO.h"
#include "RhSensor.h"
#include "TempSensor.h"
#include "controller/CO2Controller.h"
#include "hardware/timer.h"

#include "controller/ControllerEnums.h"

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


void dummy_task(void *params) {

    auto controllerQueue = static_cast<QueueHandle_t>(params);

    sensorData data;

    while (true) {
        if (xQueueReceive(controllerQueue, &data, portMAX_DELAY) == pdPASS) {
            printf("CO2: %.1f\n", data.co2);
            printf("TEMP: %.1f\n", data.temp);
            printf("RH: %.1f\n", data.rh);
        }
    }
}


int main() {

    stdio_init_all();

    auto guard{std::make_shared<Fmutex>()}; // mutex to guard modbus read/write

    auto uart{std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE, STOP_BITS)};
    auto rtu_client{std::make_shared<ModbusClient>(uart)};

    std::array<std::shared_ptr<SensorInterface>, 3> sensors = {
        std::make_shared<CO2Sensor>(rtu_client, 240, 256, 2049),
        std::make_shared<TempSensor>(rtu_client, 241, 257, 512),
        std::make_shared<RhSensor>(rtu_client, 241, 256, 512)
    };

    std::array<std::shared_ptr<ActuatorsInterface>, 2> actuators = {
        std::make_shared<ProdualMIO>(rtu_client, 1, 0, 4),
        std::make_shared<CO2Injection>(27)
    };

    QueueHandle_t controllerQueue = xQueueCreate(3, sizeof(int)); // this could be used to send commands to controller task
    QueueHandle_t displayQueue = xQueueCreate(3, sizeof(sensorData)); // for controller -> userInterfaceTask
    QueueHandle_t cloudQueue = xQueueCreate(3, sizeof(sensorData)); // for controller -> cloudTask

    CO2Controller controller(sensors, actuators, guard, controllerQueue, displayQueue, cloudQueue);

    printf("\nBoot\n");

    // dummy task to see that we can pass values from controller -> dummyTask
    xTaskCreate(dummy_task, "DUMMY_PRINT", 1024, displayQueue, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();

    while(1){};
}