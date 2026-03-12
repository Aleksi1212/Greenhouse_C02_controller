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
#include "event_groups.h"
#include "ProdualMIO.h"
#include "RhSensor.h"
#include "TempSensor.h"
#include "thingspeak.h"
#include "controller/CO2Controller.h"
#include "hardware/timer.h"

#include "controller/ControllerEnums.h"
#include "storage/ConfigStorage.h"
#include "ui/UITask.h"

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

    stdio_init_all();

    static auto guard{std::make_shared<Fmutex>()}; // mutex to guard modbus read/write

    static auto uart{std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE, STOP_BITS)};
    static auto rtu_client{std::make_shared<ModbusClient>(uart)};

    static std::array<std::shared_ptr<SensorInterface>, 3> sensors = {
        std::make_shared<CO2Sensor>(rtu_client, 240, 256, 2049),
        std::make_shared<TempSensor>(rtu_client, 241, 257, 512),
        std::make_shared<RhSensor>(rtu_client, 241, 256, 512)
    };

    static std::array<std::shared_ptr<ActuatorsInterface>, 2> actuators = {
        std::make_shared<ProdualMIO>(rtu_client, 1, 0, 4),
        std::make_shared<CO2Injection>(27)
    };

    QueueHandle_t controllerQueue = xQueueCreate(3, sizeof(int)); // this could be used to send commands to controller task
    QueueHandle_t displayQueue = xQueueCreate(3, sizeof(sensorData)); // for controller -> userInterfaceTask
    QueueHandle_t cloudQueue = xQueueCreate(3, sizeof(sensorData)); // for controller -> cloudTask
    QueueHandle_t wiFiQueue = xQueueCreate(3, sizeof(wifi_config_t)); // for UITask -> cloudTask

    // create eventGroup -> cloud set bit -> controller wait bit
    EventGroupHandle_t eventGroup = xEventGroupCreate();

    auto eeprom = std::make_shared<Eeprom>(0, 0x50); // create eeprom instance
    auto storage = std::make_shared<ConfigStorage>(eeprom);  // configuration storage

    UITask uiTask(displayQueue, controllerQueue, wiFiQueue, eventGroup, storage);

    ThingSpeak thingSpeak(cloudQueue, controllerQueue, wiFiQueue, eventGroup);

    CO2Controller controller(sensors, actuators, guard, controllerQueue, displayQueue, cloudQueue, eventGroup);

    printf("\nBoot\n");

    vTaskStartScheduler();

    while (1) {};
}