//
// Created by Mmiud on 3/4/2026.
//

#ifndef CONFIGSTORAGE_H
#define CONFIGSTORAGE_H

#include "Eeprom.h"

struct wifiConfig {
    std::string SSID;
    std::string PASSWORD;
};

class ConfigStorage {
public:
    explicit ConfigStorage(std::shared_ptr<Eeprom> eeprom);

    bool setCo2Level(uint16_t level);
    uint16_t getCo2Level();

    bool setWifiConfig(wifiConfig info);
    wifiConfig getWifiConfig();

private:
    std::shared_ptr<Eeprom> eeprom;

    uint16_t CO2_ADDR = 0x0000;
    uint16_t SSID_ADDR = 0x0040;
    uint16_t PASSWORD_ADDR = 0x0080;

    uint16_t crc(std::span<const uint8_t> bytes);
};



#endif //CONFIGSTORAGE_H
