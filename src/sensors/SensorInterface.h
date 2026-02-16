//
// Created by Mmiud on 2/11/2026.
//

#ifndef SENSORINTERFACE_H
#define SENSORINTERFACE_H



// defines interface for all the sensors

class SensorInterface {
public:
    virtual float readValue() = 0;
};



#endif //SENSORINTERFACE_H
