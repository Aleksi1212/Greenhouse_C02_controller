//
// Created by Mmiud on 2/12/2026.
//

#include "ProdualMIO.h"

ProdualMIO::ProdualMIO(std::shared_ptr<ModbusClient> client, int devAddr, int writeAddr, int readAddr) :
    controlSpeed(client, devAddr, writeAddr),
    checkRotation(client, devAddr, readAddr, false)
{}

void ProdualMIO::set(uint16_t value) {
    controlSpeed.write(value);
}

bool ProdualMIO::getStatus() {
    uint16_t count = 0;
    if (checkRotation.read() == 0) ++count;
    vTaskDelay(5);
    if (checkRotation.read() == 0) ++count;

    if (count == 2) {
        return true;
    }
    return false;

    /*value = checkRotation.read();
    printf("MIO first value: %d\n", value);
    vTaskDelay(5);
    value = checkRotation.read();
    printf("MIO second value: %d\n", value); */
}




