

#include "Eeprom.h"
#include <vector>
#include <algorithm>
#include <iterator>
#include <span>

#define MAX_BYTES 2048
#define ENTRY_SIZE 64

Eeprom::Eeprom(uint8_t bus_nr, uint8_t devAddress) : devAdd(devAddress)
{
    i2cDev = std::make_shared<PicoI2C>(bus_nr);

}

bool Eeprom::writeBytes(uint16_t address, std::span<const uint8_t> data) {
    std::vector<uint8_t> buffer;
    uint bytesWritten = 0;

    if (data.size() > ENTRY_SIZE) { // we should never write more than 64 bytes on EEPROM
        printf("EEPROM: write buffer too large\n");
        return false;
    }

    buffer.push_back((address >> 8) & 0xFF);
    buffer.push_back(address & 0xFF);

    buffer.insert(buffer.end(), data.begin(), data.end());

    bytesWritten = i2cDev->write(devAdd, buffer.data(), buffer.size());

    sleep_ms(5);

    return bytesWritten == buffer.size();
}

bool Eeprom::readBytes(uint16_t address, std::span<uint8_t> buffer) {
    std::vector<uint8_t> addr;
    uint bytesRead = 0;

    if (buffer.size() > ENTRY_SIZE) { // we should read max 64 bytes from EEPROM
        printf("EEPROM: read buffer too large\n");
        return false;
    }

    addr.push_back((address >> 8) & 0xFF);
    addr.push_back(address & 0xFF);

    bytesRead = i2cDev->transaction(devAdd, addr.data(), addr.size(), buffer.data(), buffer.size());
    sleep_ms(5);

    return bytesRead == (addr.size() + buffer.size());
}

bool Eeprom::emptyEeprom() {
    uint16_t address = 0;
    std::vector<uint8_t> buffer(64,0);

    while (address < MAX_BYTES) {
        writeBytes(address, buffer);
        address += ENTRY_SIZE;
    }

    return true;
}





