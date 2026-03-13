

#ifndef SENSORINTERFACE_H
#define SENSORINTERFACE_H



// defines interface for all the sensors

class SensorInterface {
public:
    virtual ~SensorInterface() = default;

    virtual float readValue() = 0;
};



#endif //SENSORINTERFACE_H
