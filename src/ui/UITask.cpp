//
// Created by renek on 04/03/2026.
//

#include "UITask.h"

#include <iostream>
#include <chrono>
#include <cstdio>
#include "PicoI2C.h"

#define ROT_A_PIN 10
#define ROT_B_PIN  11
#define ROT_BUTTON_PIN 12

// co2 level settings v
#define SETPOINT_STEP   10
#define SETPOINT_MAX    1500
#define SETPOINT_MIN    0

// timeouts
#define SSID_WIFI_TIMEOUT 5000
#define CONFIRM_TIMEOUT 5000
#define CO2_SET_LEVEL_TIMEOUT 10000
#define START_UP_TIMEOUT 30000

// ascii table min max values
#define ASCII_MIN 33
#define ASCII_MAX 122

// oled display
#define ROW_MAX_CHAR 16
#define WIFI_MAX_SIZE 60

UITask::UITask(QueueHandle_t displayQueue, QueueHandle_t controllerQueue, QueueHandle_t wifiQueue, EventGroupHandle_t eventGroup, std::shared_ptr<ConfigStorage> storage)
    : storage(storage), displayQueue(displayQueue), controllerQueue(controllerQueue), wifiQueue(wifiQueue), eventGroup(eventGroup),
      encoder(ROT_A_PIN, ROT_B_PIN, ROT_BUTTON_PIN)
{
    xTaskCreate(UITask::runner, "UI", 1024, this, tskIDLE_PRIORITY + 1, &handle);
}

void UITask::runner(void *params) {
    const auto instance = static_cast<UITask *>(params);
    instance->run();
}

void UITask::run() {

    auto i2c = std::make_shared<PicoI2C>(1, 400000);
    display = std::make_shared<ssd1306os>(i2c);
    display->fill(0);
    display->show();

    while (true) {
        sensorData data;

        if (xQueueReceive(displayQueue, &data, 0) == pdPASS) {
            latestData = data;
        }

        handleInput();
        checkTimeOut();
        //printf("inside ui task\n");

        display->fill(0);
        if (currentScreen == Screen::WIFI_MENU) {
            drawWifiMenuScreen();
        } else if (currentScreen == Screen::WIFI_SAVED) {
            drawWifiSavedScreen();
        } else if (currentScreen == Screen::WIFI_NEW_SSID) {
            drawWifiNewSSIDScreen();
        } else if (currentScreen == Screen::WIFI_NEW_PWD) {
            drawWifiNewPwdScreen();
        } else if (currentScreen == Screen::WIFI_CONFIRM){
            drawWifiConfirmScreen();
        } else if (currentScreen == Screen::WIFI_CONNECTING) {
            drawWifiConnecting();
        } else if (currentScreen == Screen::WIFI_ERROR) {
            drawWifiConnectionError();
        } else if (currentScreen == Screen::HOME) {
            drawHomeScreen();
        } else if (currentScreen == Screen::SETPOINT_SELECT) {
            drawSetpointSelectScreen();
        } else if (currentScreen == Screen::SETPOINT_SAVED) {
            drawSetpointSavedScreen();
        } else {
            drawSetpointScreen();
        }
        display->show();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void UITask::handleInput() {
    const int delta = encoder.getDelta();

    if (currentScreen == Screen::WIFI_MENU && delta != 0) {
        wifiMenuSelection += (delta > 0) ? 1 : -1;
        if (wifiMenuSelection < 0) wifiMenuSelection = 0;
        if (wifiMenuSelection > 1) wifiMenuSelection = 1;
        lastActive = xTaskGetTickCount();
    }

    if (currentScreen == Screen::SETPOINT_SELECT && delta != 0) {
        spMenuSelection += (delta > 0) ? 1 : -1;
        if (spMenuSelection < 0) spMenuSelection = 0;
        if (spMenuSelection > 1) spMenuSelection = 1;
    }

    if (currentScreen == Screen::EDIT_SETPOINT && delta != 0) {
        editValue += delta * SETPOINT_STEP;
        if (editValue < SETPOINT_MIN) editValue = SETPOINT_MIN;
        if (editValue > SETPOINT_MAX) editValue = SETPOINT_MAX;
        lastActive = xTaskGetTickCount();
    }

    if ((currentScreen == Screen::WIFI_NEW_SSID || currentScreen == Screen::WIFI_NEW_PWD) && delta != 0) {
        if (delta < 0) currentChar -= 1;
        else currentChar += 1;
        if (currentChar < ASCII_MIN) currentChar = ASCII_MAX; // ascii table max and min values 33 - 122
        if (currentChar > ASCII_MAX) currentChar = ASCII_MIN;
        lastActive = xTaskGetTickCount();
    }

    if (encoder.wasButtonPressed()) {

        if (currentScreen == Screen::WIFI_MENU) {
            currentScreen = (wifiMenuSelection == 0) ? Screen::WIFI_SAVED : Screen::WIFI_NEW_SSID;

        } else if (currentScreen == Screen::WIFI_SAVED) {
            getInfoFromMemory();
            currentScreen = Screen::WIFI_CONNECTING;

        } else if (currentScreen == Screen::WIFI_NEW_SSID || currentScreen == Screen::WIFI_NEW_PWD){
            if (wifiInfo.length() <= WIFI_MAX_SIZE) wifiInfo.push_back(currentChar);
            else currentScreen = Screen::WIFI_CONFIRM;


        } else if (currentScreen == Screen::WIFI_CONFIRM) {
            if (!setWifiPwd) {
                snprintf(wifiConfigInfo.ssid, sizeof(wifiConfigInfo.ssid), "%s", wifiInfo.c_str());
                currentScreen = Screen::WIFI_NEW_PWD;
                wifiInfo.clear();
            }
            else {
                snprintf(wifiConfigInfo.pwd, sizeof(wifiConfigInfo.pwd), "%s", wifiInfo.c_str());
                if (xQueueSendToBack(wifiQueue, &wifiConfigInfo, 0) == pdPASS) printf("WIFI INFO SENT TO CLOUD");
                setWifiPwd = false;
                currentScreen = Screen::WIFI_CONNECTING;
            }

        } else if (currentScreen == Screen::SETPOINT_SELECT) {
            editValue = currentSetpoint;
            currentScreen = (spMenuSelection == 0) ? Screen::SETPOINT_SAVED : Screen::EDIT_SETPOINT;

        } else if (currentScreen == Screen::SETPOINT_SAVED) {
            int level = storage->getCo2Level();
            if (level == 0xFFFF) level = 1200; // if eeprom read fails then we set the default co2 level.
            if (xQueueSendToBack(controllerQueue, &level, 0) == pdPASS) printf("SETPOINT SENT TO CONTROLLER");
            currentScreen = Screen::HOME;
        }

        else if (currentScreen == Screen::WIFI_ERROR) {
            xEventGroupSetBits(eventGroup, EVENT_BIT_2);
            currentScreen = Screen::SETPOINT_SELECT;

        } else if (currentScreen == Screen::HOME) {
            editValue = currentSetpoint;
            currentScreen = Screen::EDIT_SETPOINT;

        } else if (currentScreen == Screen::EDIT_SETPOINT){
            currentSetpoint = editValue; // this is for EDIT_SETPOINT
            //const auto sp = static_cast<float>(editValue);
            xQueueSend(controllerQueue, &currentSetpoint, 0);
            storage->setCo2Level(currentSetpoint);
            currentScreen = Screen::HOME;
        }
        lastActive = xTaskGetTickCount();
    }
}

void UITask::checkTimeOut() {

    if (currentScreen == Screen::WIFI_NEW_SSID || currentScreen == Screen::WIFI_NEW_PWD) {  // timeout for WIFI_NEW_SSID and NEW_PWD screens
        if ((xTaskGetTickCount() - lastActive) > pdMS_TO_TICKS(SSID_WIFI_TIMEOUT)) {
            if (currentScreen == Screen::WIFI_NEW_PWD) setWifiPwd = true;
            currentScreen = Screen::WIFI_CONFIRM;
            lastActive = xTaskGetTickCount();
        }
    }

    else if (currentScreen == Screen::WIFI_CONFIRM) { // timeout for WIFI_CONFIRM screen
        if ((xTaskGetTickCount() - lastActive) > pdMS_TO_TICKS(CONFIRM_TIMEOUT)) {
            if (setWifiPwd) currentScreen = Screen::WIFI_NEW_PWD;
            else currentScreen = Screen::WIFI_NEW_SSID;
            wifiInfo.clear();
            lastActive = xTaskGetTickCount();
        }
    }

    else if (currentScreen == Screen::EDIT_SETPOINT) {
        if ((xTaskGetTickCount() - lastActive) > pdMS_TO_TICKS(CO2_SET_LEVEL_TIMEOUT)) { // timeout for CO2_SET_POINT
            currentSetpoint = 1200;
            storage->setCo2Level(currentSetpoint);
            xQueueSend(controllerQueue, &currentSetpoint, 0);
            currentScreen = Screen::HOME;
        }
    }

    else if (currentScreen == Screen::WIFI_MENU) {
        if ((xTaskGetTickCount() - lastActive) > pdMS_TO_TICKS(START_UP_TIMEOUT)) {
            checkDefault = true;
            checkLastConfigInfo();
            //currentScreen = Screen::HOME;
        }
    }
}


void UITask::drawHomeScreen() {
    char buf[17];

    display->text("- READINGS -", 14, 8);

    snprintf(buf, sizeof(buf), "CO2: %4.0f ppm", latestData.co2.value);
    display->text(buf, 0, 20);

    snprintf(buf, sizeof(buf), "SP:  %4.0f ppm", latestData.co2Set.value);
    display->text(buf, 0, 30);

    snprintf(buf, sizeof(buf), "Temp: %5.1f C", latestData.temp.value);
    display->text(buf, 0, 40);

    snprintf(buf, sizeof(buf), "RH:    %4.1f %%", latestData.rh.value);
    display->text(buf, 0, 50);
}

void UITask::drawSetpointScreen() {
    char buf[20];

    display->text("Set CO2 Target", 8, 8);

    snprintf(buf, sizeof(buf), "> %4d ppm <", editValue);
    display->text(buf, 16, 28);

    display->text("max: 1500 ppm", 12, 44);
    display->text("Press to confirm", 0, 56);
}

void UITask::drawWifiMenuScreen() {
    display->text("CONNECT WIRELESS", 0, 8);

    display->text((wifiMenuSelection == 0) ? "> Saved login" : "  Saved login", 0, 24);
    display->text((wifiMenuSelection == 1) ? "> Enter new"   : "  Enter new",   0, 40);
}

void UITask::drawWifiSavedScreen() {
    // PLACEHOLDER!!! for getting wifi info from eeprom
    display->text("CHECKING EEPROM", 0, 20);
    //display->text("config info...", 0, 32);
    display->text("Press to cont.", 0, 50);
}

void UITask::drawWifiNewSSIDScreen() {
    display->text("New SSID: ", 0, 8);
    displayWifiSetInfo();
}

void UITask::drawWifiNewPwdScreen() {
    display->text("New password: ", 0, 8);
    displayWifiSetInfo();
}

void UITask::drawWifiConfirmScreen() {
    display->text("SAVING: ", 36, 10);
    display->text(wifiInfo.c_str(), 0, 30 );
    if (wifiInfo.length() >= ROW_MAX_CHAR) {
        std::string subStr = wifiInfo.substr(ROW_MAX_CHAR);
        display->text(subStr.c_str(), 0, 40);
    }
    display->text("Press to save", 0, 50);
}

void UITask::drawWifiConnectionError() {
    std::string msg = "FAILED TO CONNECT! Press to continue without connection or restart to try again.";
    int row = 10;
    for (int i = 0; i < msg.length(); i += ROW_MAX_CHAR) {
        std::string subStr = msg.substr(i, ROW_MAX_CHAR);
        display->text(subStr.c_str(), 0, row);
        row += 10;
    }
}

void UITask::drawWifiConnecting() {
    display->text("Connecting...", 10, 25);
    checkEventBits();
}

void UITask::drawSetpointSavedScreen() {
    // PLACEHOLDER!!! for getting wifi info from eeprom
    display->text("CHECKING EEPROM", 0, 20);
    //display->text("config info...", 0, 32);
    display->text("Press to cont.", 0, 50);
}

void UITask::drawSetpointSelectScreen() {
    display->text("SET CO2 TARGET", 8, 8);
    display->text((spMenuSelection == 0) ? "> Saved SP" : "  Saved SP", 0, 24);
    display->text((spMenuSelection == 1) ? "> Enter new"   : "  Enter new",   0, 40);
}

void UITask::displayWifiSetInfo() {
    char buf[16];
    snprintf(buf, sizeof(buf), "-> %c <-", currentChar);
    display->text(buf, 36, 20);
    display->text(wifiInfo.c_str(), 0, 30 );
    if (wifiInfo.length() >= ROW_MAX_CHAR) {
        std::string subStr = wifiInfo.substr(ROW_MAX_CHAR);
        display->text(subStr.c_str(), 0, 40);
    }
}

void UITask::saveInfoToMemory() {
     wifiConfig info {
        .SSID = wifiConfigInfo.ssid,
        .PASSWORD = wifiConfigInfo.pwd
    };

    storage->setWifiConfig(info);
    printf("saved wifi info on EEPROM\n");
}

void UITask::checkEventBits() {
    EventBits_t setBits = xEventGroupGetBits(eventGroup);
    if ((setBits & EVENT_BIT_1) != 0) {
        printf("connection failed.");
        currentScreen = Screen::WIFI_ERROR;
    }
    else if ((setBits & EVENT_BIT_0) != 0) {
        saveInfoToMemory();
        if (!checkDefault) currentScreen = Screen::SETPOINT_SELECT;
        lastActive = xTaskGetTickCount();
        wifiInfo.clear();
        std::cout << "SAVED INFO: " << wifiConfigInfo.ssid << " " << wifiConfigInfo.pwd << std::endl;
    }
}

void UITask::getInfoFromMemory() {
    wifiConfig info = storage->getWifiConfig();
    snprintf(wifiConfigInfo.pwd, sizeof(wifiConfigInfo.pwd), "%s", info.PASSWORD.c_str());
    snprintf(wifiConfigInfo.ssid, sizeof(wifiConfigInfo.ssid), "%s", info.SSID.c_str());
    if (xQueueSendToBack(wifiQueue, &wifiConfigInfo, 0) == pdPASS) printf("WIFI INFO SENT TO CLOUD");

}


void UITask::checkLastConfigInfo() {
    getInfoFromMemory();
    checkEventBits();
    int level = storage->getCo2Level();
    if (level == 0xFFFF) level = 1200; // if eeprom read fails then we set the default co2 level.
    if (xQueueSendToBack(controllerQueue, &level, 0) == pdPASS) printf("SETPOINT SENT TO CONTROLLER");
    currentScreen = Screen::HOME;
}




