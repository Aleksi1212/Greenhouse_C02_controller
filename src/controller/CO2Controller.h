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
#include "queue.h"
#include "timers.h"
#include "Fmutex.h"
#include "ControllerEnums.h"


class CO2Controller {
public:
    CO2Controller(const std::array<std::shared_ptr<SensorInterface>, 3> &sensors, const std::array<std::shared_ptr<ActuatorsInterface>, 2> &actuators,
        const std::shared_ptr<Fmutex> guard, QueueHandle_t controlQueue, QueueHandle_t displayQueue, QueueHandle_t cloudQueue, int co2Level = 1200, TickType_t measureInterval = 3000);

private:
    static void runner(void *params);
    void run();
    static void valveTimerCallback(TimerHandle_t xTimer);

    std::array<std::shared_ptr<SensorInterface>, 3> sensors;
    std::array<std::shared_ptr<ActuatorsInterface>, 2> actuators;
    TaskHandle_t handle;
    QueueHandle_t controllerQueue;
    QueueHandle_t displayQueue;
    QueueHandle_t cloudQueue;
    TimerHandle_t valveTimer;

    std::shared_ptr<Fmutex> guard;

    int co2Level; // this can be changed through UI or remotely
    TickType_t measuringInterval;

    TickType_t lastValveOpenTime{};
    bool valveCanOpen{};

    float co2{};
    float temp{};
    float rh{};

    bool sensorStartUp();
    void readSensors();
    void controlFan();
    void controlValve();

};



#endif //CO2CONTROLLER_H
