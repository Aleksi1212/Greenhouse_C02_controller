//
// Created by Mmiud on 2/11/2026.
//

#include "RhSensor.h"

RhSensor::RhSensor(std::shared_ptr<ModbusClient> client, int devAddr, int regAddr, int statusRegAddr) :
    rh(client, devAddr, regAddr),
    status(client, devAddr, statusRegAddr, false)
{}

float RhSensor::readValue() {
    return rh.read() / 10.0;
}

// if we check this already with temp, probably no need to do it here again?
// in this case we would need to read temp first.
bool RhSensor::checkStatus() {
    return (status.read() == 0x0001);
}


