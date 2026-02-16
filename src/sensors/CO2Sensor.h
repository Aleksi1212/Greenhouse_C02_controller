//
// Created by Mmiud on 2/11/2026.
//

#ifndef CO2SENSOR_H
#define CO2SENSOR_H
#include "SensorInterface.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"

class CO2Sensor : public SensorInterface {
public:
    CO2Sensor(std::shared_ptr<ModbusClient> client, int devAddr, int regAddr, int statusRegAddr);
    float readValue() override;
private:
    //std::shared_ptr<ModbusClient> client;
    ModbusRegister co2;
    ModbusRegister status;

    uint16_t checkStatus();
};



#endif //CO2SENSOR_H
