//
// Created by Mmiud on 2/11/2026.
//

#include "TempSensor.h"

TempSensor::TempSensor(std::shared_ptr<ModbusClient> client, int devAddr, int regAddr, int statusRegAddr) :
    //client(client),
    temp(client, devAddr, regAddr),
    status(client, devAddr, statusRegAddr)
{}

float TempSensor::readValue() {
    if (checkStatus()) {
        return temp.read() / 10.0;
    }
    printf("Temp status not okay. One or more errors active.\n");
    return 0.0;
}

bool TempSensor::checkStatus() {
    return status.read() == 0x0001;
}