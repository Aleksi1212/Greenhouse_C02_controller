

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
    ModbusRegister rh;
    ModbusRegister status;

    bool checkStatus();
};



#endif //RHSENSOR_H
