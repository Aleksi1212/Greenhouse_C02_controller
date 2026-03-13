
#include "RhSensor.h"

RhSensor::RhSensor(std::shared_ptr<ModbusClient> client, int devAddr, int regAddr, int statusRegAddr) :
    rh(client, devAddr, regAddr),
    status(client, devAddr, statusRegAddr, false)
{}

float RhSensor::readValue() {
    return rh.read() / 10.0;
}

bool RhSensor::checkStatus() {
    return (status.read() == 0x0001);
}


