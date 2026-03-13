//
// Created by Mmiud on 3/4/2026.
//

#include "ConfigStorage.h"
#include <vector>

// use setCO2Level for storing co2level on eeprom and get for getting the previous value
// wifi config info is saved and retrieved using wifiConfig struct -> we need to typecast these c strings before passing into a queue
// each value read is validated with checksum calculation

// if reading co2level fails error value sent is 0XFFFF
// if reading wifi config fails empty strings are returned

ConfigStorage::ConfigStorage(std::shared_ptr<Eeprom> eeprom) : eeprom(eeprom) {
}

bool ConfigStorage::setCo2Level(uint16_t level) {

    printf("co2 level inside eeprom: %d", level);

    std::vector<uint8_t> buffer;
    buffer.push_back((level >> 8) & 0xFF);
    buffer.push_back(level & 0xFF);

    uint16_t checkSum = crc(buffer);
    buffer.push_back((checkSum >> 8) & 0xFF);
    buffer.push_back(checkSum & 0xFF);

    return eeprom->writeBytes(CO2_ADDR, buffer);
}

int ConfigStorage::getCo2Level() {

    std::vector<uint8_t> buffer(4); // uint16_t set level value + 2 crc bytes

    eeprom->readBytes(CO2_ADDR, buffer);

    if (crc(buffer) != 0) {
        return 0xFFFF; // error value if crc validations fails
    }
    return static_cast<uint16_t>(buffer[0] << 8) | static_cast<uint16_t>(buffer[1]);
}

bool ConfigStorage::setWifiConfig(wifiConfig info) {
    std::vector<uint8_t> ssidBuffer;
    std::vector<uint8_t> passwordBuffer;

    ssidBuffer.insert(ssidBuffer.begin(), info.SSID.begin(), info.SSID.end());
    ssidBuffer.push_back('\0');

    uint16_t checkSumSsid = crc(ssidBuffer);
    ssidBuffer.push_back((checkSumSsid >> 8) & 0xFF);
    ssidBuffer.push_back(checkSumSsid & 0xFF);

    if (!eeprom->writeBytes(SSID_ADDR, ssidBuffer)) {
        return false;
    }

    passwordBuffer.insert(passwordBuffer.begin(), info.PASSWORD.begin(), info.PASSWORD.end());
    passwordBuffer.push_back('\0');

    uint16_t checkSumPwd = crc(passwordBuffer);
    passwordBuffer.push_back((checkSumPwd >> 8) & 0xFF);
    passwordBuffer.push_back(checkSumPwd & 0xFF);

    return eeprom->writeBytes(PASSWORD_ADDR, passwordBuffer);
}

wifiConfig ConfigStorage::getWifiConfig() {
    std::vector<uint8_t> ssidBuffer(64);
    std::vector<uint8_t> pwdBuffer(64);
    std::vector<uint8_t> buffer;
    wifiConfig wifiInfo = {.SSID = "", .PASSWORD = ""};

    eeprom->readBytes(SSID_ADDR, ssidBuffer);

    if (auto it = std::ranges::find(ssidBuffer, '\0'); it != ssidBuffer.end()) { // check that null exists
        buffer.insert(buffer.begin(), ssidBuffer.begin(), it + 3); // include null and crc bytes

        if (crc(buffer) == 0) { // validate
            wifiInfo.SSID = std::string(buffer.begin(), buffer.end() - 2); // remove crc bytes
            //printf("SSID validated.\n");
        }
    }

    buffer.clear();

    eeprom->readBytes(PASSWORD_ADDR, pwdBuffer);

    if (auto it = std::ranges::find(pwdBuffer, '\0'); it != pwdBuffer.end()) { // check that null exists
        buffer.insert(buffer.begin(), pwdBuffer.begin(), it + 3);

        if (crc(buffer) == 0) {
            wifiInfo.PASSWORD = std::string(buffer.begin(), buffer.end() - 2); // remove crc bytes
        }
    }

    return wifiInfo;
}

uint16_t ConfigStorage::crc(std::span<const uint8_t> bytes) {
    uint8_t x;
    uint16_t crc = 0xFFFF;

    for (auto b : bytes) {
        x = crc >> 8 ^ b;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t) (x << 12)) ^ ((uint16_t) (x << 5)) ^ ((uint16_t) x);
    }
    //printf("inside crc check: %d\n", crc);
    return crc;
}





