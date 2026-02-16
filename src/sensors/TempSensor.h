//
// Created by Mmiud on 2/11/2026.
//

#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H
#include "SensorInterface.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"


class TempSensor : public SensorInterface {
public:
    TempSensor(std::shared_ptr<ModbusClient> client, int devAddr, int regAddr, int statusRegAddr);
    float readValue() override;
private:
    //std::shared_ptr<ModbusClient> client;
    ModbusRegister temp;
    ModbusRegister status;

    bool checkStatus();
};



#endif //TEMPSENSOR_H
