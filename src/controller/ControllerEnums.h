//
// Created by Mmiud on 2/12/2026.
//

#ifndef CONTROLLERENUMS_H
#define CONTROLLERENUMS_H

enum ActuatorIndex {
    FAN, // numbers are automatically 0 and 1?
    CO2_VALVE
};

enum SensorIndex {
    CO2,
    TEMP,
    RH
};

struct sensorData {
    float co2{};
    float temp{};
    float rh{};
};

#endif //CONTROLLERENUMS_H
