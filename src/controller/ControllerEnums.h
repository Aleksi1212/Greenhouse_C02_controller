//
// Created by Mmiud on 2/12/2026.
//

#ifndef CONTROLLERENUMS_H
#define CONTROLLERENUMS_H

#define EVENT_BIT_0 (1UL << 0UL) // cloud connected
#define EVENT_BIT_1 (1UL << 1UL) // cloud not connected
#define EVENT_BIT_2 (1UL << 2UL)

enum ActuatorIndex {
    FAN, // numbers are automatically 0 and 1?
    CO2_VALVE
};

enum SensorIndex {
    CO2,
    TEMP,
    RH
};

enum cloudIndex {
    CO2_C = 1,
    TEMP_C,
    RH_C,
    FAN_C
};

typedef struct data {
    int index;
    float value;
} data_;

typedef struct sensorData_ {
    data_ co2{};
    data_ temp{};
    data_ rh{};
    data_ fan{};
} sensorData;

typedef struct wifiConfigInfo {
    char ssid[64];
    char pwd[64];
} wifi_config_t;

#endif //CONTROLLERENUMS_H
