//
// Created by Mmiud on 2/12/2026.
//

#ifndef ACTUATORSINTERFACE_H
#define ACTUATORSINTERFACE_H
#include <stdint.h>

// creates interface for actuators

class ActuatorsInterface {
public:
    virtual void set(uint16_t value) = 0;
    virtual bool getStatus() = 0;
};



#endif //ACTUATORSINTERFACE_H
