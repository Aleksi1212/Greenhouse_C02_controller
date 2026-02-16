//
// Created by Mmiud on 2/12/2026.
//

#ifndef PRODUALMIO_H
#define PRODUALMIO_H
#include "ActuatorsInterface.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"

class ProdualMIO : public ActuatorsInterface{
public:
    ProdualMIO(std::shared_ptr<ModbusClient> client, int devAddr, int writeAddr, int readAddr);
    void set(uint16_t value) override;
    bool getStatus() override;


private:
    ModbusRegister controlSpeed;
    ModbusRegister checkRotation;
};



#endif //PRODUALMIO_H
