//
// Created by Mmiud on 2/12/2026.
//

#ifndef CO2INJECTION_H
#define CO2INJECTION_H
#include "ActuatorsInterface.h"


class CO2Injection : public ActuatorsInterface {
public:
    CO2Injection(int pinNr);
    void set(uint16_t value) override;
    bool getStatus();

private:
    int GPIOPin;
};



#endif //CO2INJECTION_H
