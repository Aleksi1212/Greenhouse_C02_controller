//
// Created by Mmiud on 2/11/2026.
//

#include "CO2Sensor.h"

CO2Sensor::CO2Sensor(std::shared_ptr<ModbusClient> client, int devAddr, int regAddr, int statusRegAddr) :
    co2(client, devAddr, regAddr),
    status(client, devAddr, statusRegAddr)
{}

float CO2Sensor::readValue() {
    if (checkStatus() != 0) {
        printf("CO2 status not okay. Error: %d\n", checkStatus());
        return 0.0;
    }
    return co2.read() / 1.0;
}

uint16_t CO2Sensor::checkStatus() {
    /*uint16_t value = status.read();
    printf("co2 check value: %d\n", value);*/
    return status.read();
}


