//
// Created by Mmiud on 2/12/2026.
//

#include "CO2Controller.h"
#include <vector>
#include <sys/stat.h>

CO2Controller::CO2Controller(const std::array<std::shared_ptr<SensorInterface>, 3> &sensors, const std::array<std::shared_ptr<ActuatorsInterface>, 2> &actuators) :
    sensors(sensors), actuators(actuators)
{
    xTaskCreate(CO2Controller::runner, "CONTROLLER", 1024, this, tskIDLE_PRIORITY + 1, &handle);
}

void CO2Controller::runner(void *params) {
    auto instance = static_cast<CO2Controller *>(params);
    instance->run();
}

void CO2Controller::run() {
    while (true) {
        readSensors();
        checkThresholds();
        checkMotor();
        vTaskDelay(3000);
    }
}

void CO2Controller::readSensors() {
    co2 = sensors[CO2]->readValue();
    printf("CO2: %.1f\n", co2);
    vTaskDelay(5);
    temp = sensors[TEMP]->readValue();
    printf("T: %.1f\n", temp);
    vTaskDelay(5);
    rh = sensors[RH]->readValue();
    printf("RH: %.1f\n", rh);
}

void CO2Controller::checkFan() {
    fanOn = actuators[FAN]->getStatus();
}

void CO2Controller::checkThresholds() {
    if (co2 < 600) {
        printf("CO2 level too low. Opening valve.\n");
        actuators[FAN]->set(0);
        controlValve(2000);
        vTaskDelay(1000);
    }
    if (co2 > 1800) {
        if (!fanOn) {
            printf("CO2 level too high! Starting fan.\n");
            actuators[FAN]->set(500);
        }
    }
    else {
        if (fanOn) {
            printf("CO2 level ok. Stopping fan.\n");
            actuators[FAN]->set(0);
        }
    }
}

void CO2Controller::controlValve(size_t time) {
    actuators[CO2_VALVE]->set(1);
    vTaskDelay(time);
    actuators[CO2_VALVE]->set(0);
}

void CO2Controller::checkMotor() {
    fanOn = actuators[FAN] -> getStatus();

    if (!fanOn) {
        printf("Motor stopped. \n");
    }
    else {
        printf("Motor moving. \n");
    }
}




