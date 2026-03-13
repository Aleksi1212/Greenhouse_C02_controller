
#ifndef ACTUATORSINTERFACE_H
#define ACTUATORSINTERFACE_H
#include <stdint.h>

// creates interface for actuators

class ActuatorsInterface {
public:
    virtual ~ActuatorsInterface() = default;

    virtual void set(uint16_t value) = 0;
    virtual bool getStatus() = 0;
};



#endif //ACTUATORSINTERFACE_H
