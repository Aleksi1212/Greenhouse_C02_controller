//
// Created by Mmiud on 2/20/2026.
//

#ifndef EEPROM_H
#define EEPROM_H

#include "PicoI2C.h"
#include <span>
#include <memory>



class Eeprom {
public:
    Eeprom(uint8_t bus_nr, uint8_t devAddress);

    bool writeBytes(uint16_t address, std::span<const uint8_t> data);
    bool readBytes(uint16_t address, std::span<uint8_t> buffer);
    bool emptyEeprom();

private:
    std::shared_ptr<PicoI2C> i2cDev;
    uint8_t devAdd;
};



#endif //EEPROM_H
