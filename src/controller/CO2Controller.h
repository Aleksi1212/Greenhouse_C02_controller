//
// Created by Mmiud on 2/12/2026.
//

#ifndef CO2CONTROLLER_H
#define CO2CONTROLLER_H

#include <array>
#include <memory>
#include "SensorInterface.h"
#include "ActuatorsInterface.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ControllerEnums.h"


class CO2Controller {
public:
    CO2Controller(const std::array<std::shared_ptr<SensorInterface>, 3> &sensors, const std::array<std::shared_ptr<ActuatorsInterface>, 2> &actuators);

private:
    static void runner(void *params);
    void run();

    std::array<std::shared_ptr<SensorInterface>, 3> sensors;
    std::array<std::shared_ptr<ActuatorsInterface>, 2> actuators;
    TaskHandle_t handle;

    int upperLimit;
    int lowerLimit;

    float co2;
    float temp;
    float rh;
    bool fanOn;

    void readSensors();
    void checkFan();
    void checkThresholds();
    void controlValve(size_t time);
    void checkMotor();

};



#endif //CO2CONTROLLER_H
