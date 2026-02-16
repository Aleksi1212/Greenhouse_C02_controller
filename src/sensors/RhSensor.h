//
// Created by Mmiud on 2/11/2026.
//

#ifndef RHSENSOR_H
#define RHSENSOR_H
#include <cstdint>

#include "SensorInterface.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"


class RhSensor : public SensorInterface {
public:
    RhSensor(std::shared_ptr<ModbusClient> client, int devAddr, int regAddr, int statusRegAddr);
    float readValue() override;

private:
    //std::shared_ptr<ModbusClient> client;
    ModbusRegister rh;
    ModbusRegister status;

    bool checkStatus();
};



#endif //RHSENSOR_H
