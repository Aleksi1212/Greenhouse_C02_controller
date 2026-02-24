//
// Created by Mmiud on 2/12/2026.
//

#include "CO2Controller.h"
#include <vector>
#include <sys/stat.h>
#include <mutex>

#include "queue.h"

#define FAN_ON_MAX 1000
#define START_UP_MAX_TIME 180000
#define CO2_WARM_UP_TIME 12000
#define VALVE_OPEN_TIME 1000

CO2Controller::CO2Controller(const std::array<std::shared_ptr<SensorInterface>, 3> &sensors, const std::array<std::shared_ptr<ActuatorsInterface>, 2> &actuators, const std::shared_ptr<Fmutex> guard, QueueHandle_t controlQueue, QueueHandle_t displayQueue, QueueHandle_t cloudQueue, float co2Level, TickType_t measureInterval) :
    sensors(sensors), actuators(actuators), guard(guard), controllerQueue(controlQueue), displayQueue(displayQueue), cloudQueue(cloudQueue), co2Level(co2Level), measuringInterval(measureInterval)
{
    measurementCount = 1; // set this to one so we can open valve during first round if needed
    valveTimer = xTimerCreate("VALVE_TIMER", VALVE_OPEN_TIME, pdFALSE, this, valveTimerCallback);
    xTaskCreate(CO2Controller::runner, "CONTROLLER", 4096, this, tskIDLE_PRIORITY + 1, &handle);
}

// timer to control co2 valve -> we can open valve max 2s
void CO2Controller::valveTimerCallback(TimerHandle_t xTimer) {
    auto instance = static_cast<CO2Controller*>(pvTimerGetTimerID(xTimer));
    instance->actuators[CO2_VALVE]->set(0);
    printf("valve closed\n");
}


void CO2Controller::runner(void *params) {
    auto instance = static_cast<CO2Controller *>(params);
    instance->run();
}

// controller main task
void CO2Controller::run() {

    if (!sensorStartUp()) printf("FAILED TO INITIALIZE SENSORS\n"); // suspend or reboot or something here

    while (true) {

        readSensors();
        controlFan();
        controlValve();

        //if (xQueueReceive(controllerQueue, &co2Level, measuringInterval) == pdPASS)
        vTaskDelay(measuringInterval); // replace this with xQueueReceive and measuring interval is delay
    }
}

void CO2Controller::controlValve() {

    if (measurementCount >= 2) {
        if (co2 <= co2Level - 20) {
            std::lock_guard<Fmutex> lock(*guard); // lock modbus write

            printf("valve opened\n");
            actuators[CO2_VALVE]->set(1); // open valve
            xTimerStart(valveTimer, 0);

            /*vTaskDelay(VALVE_OPEN_TIME);
            actuators[CO2_VALVE]->set(0);
            printf("valve closed\n");*/

            measurementCount = 0;
        }
    }
}


void CO2Controller::controlFan() {
    std::lock_guard<Fmutex> lock(*guard); // lock modbus read and write

    //bool fanOff = actuators[FAN]->getStatus();
    if (co2 >= 2000) {
        if (actuators[FAN]->getStatus()) {
            actuators[FAN]->set(FAN_ON_MAX); // set fan to 100%
        }
    }
    else {
        if (!actuators[FAN]->getStatus()) actuators[FAN]->set(0); // set fan off
    }
}


void CO2Controller::readSensors() {
    std::lock_guard<Fmutex> lock(*guard); // lock modbus read

    co2 = sensors[CO2]->readValue();
    //printf("CO2: %.1f\n", co2);
    temp = sensors[TEMP]->readValue();
    //printf("T: %.1f\n", temp);
    rh = sensors[RH]->readValue();
    //printf("RH: %.1f\n", rh);

    // sensorData struct is passed to queues if data is accurate
    sensorData data = {
        .co2 = co2,
        .temp = temp,
        .rh = rh
    };

    // if data is not accurate we don't send it forward
    if (co2 == 0.0 || temp == 0.0 || rh == 0.0) {
        printf("data not accurate\n");
    } else {
        xQueueSend(displayQueue, &data, 0);
        //xQueueSend(cloudQueue, &data, 10);
    }
    ++measurementCount;

}

bool CO2Controller::sensorStartUp() {
    TickType_t start = xTaskGetTickCount();
    printf("CO2 Sensor warming up\n");
    vTaskDelay(CO2_WARM_UP_TIME); // warm up time 12 seconds according to data sheet

    do {
        std::lock_guard<Fmutex> lock(*guard); // lock modbus reads

        co2 = sensors[CO2]->readValue();;

        vTaskDelay(2000);
    } while (co2 == 0 && (xTaskGetTickCount() - start) <= START_UP_MAX_TIME); // wait max 3 min to warm up - according to data sheet readings should be accurate after two min

    return co2 != 0;
}








