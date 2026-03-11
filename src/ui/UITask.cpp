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

// ascii table min max values
#define ASCII_MIN 33
#define ASCII_MAX 122

// oled display
#define ROW_MAX_CHAR 16
#define WIFI_MAX_SIZE 60

UITask::UITask(QueueHandle_t displayQueue, QueueHandle_t controllerQueue)
    : displayQueue(displayQueue), controllerQueue(controllerQueue),
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
        // "drains" the queue keeping only the newest reading
        sensorData data;
        /*while (xQueueReceive(displayQueue, &data, 0) == pdPASS) {
            latestData = data;
            printf("received data in ui: %f\n", latestData.co2);
        }*/

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
        } else if (currentScreen == Screen::HOME) {
            drawHomeScreen();
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
            //lastActive = xTaskGetTickCount();
            currentScreen = (wifiMenuSelection == 0) ? Screen::WIFI_SAVED : Screen::WIFI_NEW_SSID;

        } else if (currentScreen == Screen::WIFI_SAVED) {
            currentScreen = Screen::HOME;

        } else if (currentScreen == Screen::WIFI_NEW_SSID || currentScreen == Screen::WIFI_NEW_PWD){
            //lastActive = xTaskGetTickCount();
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
                //save eeprom
                //send to queue
                setWifiPwd = false;
                currentScreen = Screen::EDIT_SETPOINT;
                wifiInfo.clear();
                std::cout << "SAVED INFO: " << wifiConfigInfo.ssid << " " << wifiConfigInfo.pwd << std::endl;
            }
            //lastActive = xTaskGetTickCount();

        } else if (currentScreen == Screen::HOME) {
            editValue = currentSetpoint;
            currentScreen = Screen::EDIT_SETPOINT;

        } else {
            currentSetpoint = editValue; // this is for EDIT_SETPOINT
            const auto sp = static_cast<float>(editValue);
            xQueueSend(controllerQueue, &sp, 0);
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
            //save default to eeprom
            //set default to queue and pass to controller
            currentScreen = Screen::HOME;
        }
    }
}


void UITask::drawHomeScreen() {
    char buf[17];

    display->text("- READINGS -", 14, 8);

    snprintf(buf, sizeof(buf), "CO2: %4.0f ppm", latestData.co2);
    display->text(buf, 0, 20);

    snprintf(buf, sizeof(buf), "SP:  %4d ppm", currentSetpoint);
    display->text(buf, 0, 30);

    snprintf(buf, sizeof(buf), "Temp: %5.1f C", latestData.temp);
    display->text(buf, 0, 40);

    snprintf(buf, sizeof(buf), "RH:    %4.1f %%", latestData.rh);
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
    display->text("Placeholder for", 0, 20);
    display->text("saved", 0, 32);
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


void UITask::drawWifiConfirmScreen() {
    display->text("SAVING: ", 36, 10);
    display->text(wifiInfo.c_str(), 0, 30 );
    if (wifiInfo.length() >= ROW_MAX_CHAR) {
        std::string subStr = wifiInfo.substr(ROW_MAX_CHAR);
        display->text(subStr.c_str(), 0, 40);
    }
    display->text("Press to save", 0, 50);
}
