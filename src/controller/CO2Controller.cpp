//
// Created by Mmiud on 2/12/2026.
//

#include "CO2Controller.h"
#include <vector>
#include <sys/stat.h>
#include <mutex>

#include "queue.h"

#define VALVE_WAIT_TIME 60000
#define FAN_ON_MAX 1000
#define START_UP_MAX_TIME 300000
#define CO2_WARM_UP_TIME 15000
#define VALVE_OPEN_MIN_TIME 800
#define VALVE_OPEN_MAX_TIME 2000
#define VALVE_OPEN_TIME 1500

CO2Controller::CO2Controller(const std::array<std::shared_ptr<SensorInterface>, 3> &sensors, const std::array<std::shared_ptr<ActuatorsInterface>, 2> &actuators, const std::shared_ptr<Fmutex> guard, QueueHandle_t controlQueue, QueueHandle_t displayQueue, QueueHandle_t cloudQueue, int co2Level, TickType_t measureInterval) :
    sensors(sensors), actuators(actuators), guard(guard), controllerQueue(controlQueue), displayQueue(displayQueue), cloudQueue(cloudQueue), co2Level(co2Level), measuringInterval(measureInterval)
{
    valveTimer = xTimerCreate("VALVE_TIMER", 1000, pdFALSE, this, valveTimerCallback);
    xTaskCreate(CO2Controller::runner, "CONTROLLER", 4096, this, tskIDLE_PRIORITY + 1, &handle);
}

// timer to control co2 valve -> we can open valve max 2s
void CO2Controller::valveTimerCallback(TimerHandle_t xTimer) { // todo check keijo is it okay to use software timer or should this be hardware
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

    valveCanOpen = true;

    bool sensorsOK = sensorStartUp(); // warm up co2 sensor until we get accurate results

    if (!sensorsOK) printf("FAILED TO INITIALIZE SENSORS\n"); // suspend or reboot or something here

    TickType_t lastWakeTime = xTaskGetTickCount();

    while (true) {

        // see if there is values inside the queue and set new co2Level accordingly
        //if (xQueueReceive(controllerQueue, &temp, 0) == pdPASS) // other option would be to use this as our delay -> but than interval is not so accurate?

        readSensors();
        controlFan();
        controlValve();

        vTaskDelayUntil(&lastWakeTime, measuringInterval); // polling interval 3 sec (default)
    }
}

void CO2Controller::controlValve() {
    bool openValve = false;
    TickType_t valveOpenPeriod = 0;

    if (valveCanOpen) {
        if (co2 <= co2Level - 70) {
            valveOpenPeriod = VALVE_OPEN_MAX_TIME; // open valve for two secs
            openValve = true;
        }
        else if (co2 <= co2Level - 40) {
            valveOpenPeriod = VALVE_OPEN_TIME; // open valve for 1.5 secs
            openValve = true;
        }
        else if (co2 <= co2Level - 20) {
            valveOpenPeriod = VALVE_OPEN_MIN_TIME; // open valve for 0.8 secs
            openValve = true;
        }

        if (openValve) {
            std::lock_guard<Fmutex> lock(*guard); // lock modbus write

            lastValveOpenTime = xTaskGetTickCount();
            actuators[CO2_VALVE]->set(1); // open valve
            xTimerChangePeriod(valveTimer, valveOpenPeriod, 0);
            xTimerStart(valveTimer, 0);
            printf("valve opened\n");
            valveCanOpen = false;
        }

    }
    else { // todo check with keijo is it okay to poll this time or should there be a timer
        // if our sensor measurement interval is > 3sec maybe there should be a timer for checking when we can open the valve again
        if (xTaskGetTickCount() - lastValveOpenTime >= VALVE_WAIT_TIME) valveCanOpen = true; // once valve is opened we should wait for one min before opening it again
    }
}


void CO2Controller::controlFan() {
    std::lock_guard<Fmutex> lock(*guard); // lock modbus read and write

    bool fanOff = actuators[FAN]->getStatus();
    if (co2 >= 2000) {
        if (fanOff) {
            actuators[FAN]->set(FAN_ON_MAX); // set fan to 100%
        }
    }
    else {
        if (!fanOff) actuators[FAN]->set(0); // set fan off
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

}

bool CO2Controller::sensorStartUp() {
    TickType_t start = xTaskGetTickCount();
    printf("CO2 Sensor warming up\n");
    vTaskDelay(CO2_WARM_UP_TIME); // todo check warm up time from data sheet
    do {
        std::lock_guard<Fmutex> lock(*guard); // lock modbus reads

        co2 = sensors[CO2]->readValue();;
        temp = sensors[TEMP]->readValue();
        rh = sensors[RH]->readValue();
        vTaskDelay(2000);
    } while (co2 == 0 && (xTaskGetTickCount() - start) <= START_UP_MAX_TIME); // wait max 5 min to warm up

    return co2 != 0;
}








