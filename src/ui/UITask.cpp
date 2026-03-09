//
// Created by renek on 04/03/2026.
//

#include "UITask.h"
#include <cstdio>
#include "PicoI2C.h"

#define ROT_A_PIN 10
#define ROT_B_PIN  11
#define ROT_BUTTON_PIN 12

// co2 level settings v
#define SETPOINT_STEP   10
#define SETPOINT_MAX    1500
#define SETPOINT_MIN    0

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
        //printf("inside ui task\n");

        display->fill(0);
        if (currentScreen == Screen::WIFI_MENU) {
            drawWifiMenuScreen();
        } else if (currentScreen == Screen::WIFI_SAVED) {
            drawWifiSavedScreen();
        } else if (currentScreen == Screen::WIFI_NEW) {
            drawWifiNewScreen();
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
    }

    if (encoder.wasButtonPressed()) {

        if (currentScreen == Screen::WIFI_MENU) {
            currentScreen = (wifiMenuSelection == 0) ? Screen::WIFI_SAVED : Screen::WIFI_NEW;

        } else if (currentScreen == Screen::WIFI_SAVED || currentScreen == Screen::WIFI_NEW) {
            currentScreen = Screen::HOME;

        } else if (currentScreen == Screen::HOME) {
            editValue = currentSetpoint;
            currentScreen = Screen::EDIT_SETPOINT;

        } else {
            currentSetpoint = editValue;
            const auto sp = static_cast<float>(editValue);
            xQueueSend(controllerQueue, &sp, 0);
            currentScreen = Screen::HOME;
        }
    }
}

void UITask::drawHomeScreen() {
    char buf[17];

    display->text("- READINGS -", 14, 8);

    snprintf(buf, sizeof(buf), "CO2: %4.0f ppm", latestData.co2);
    display->text(buf, 0, 20);
    //printf("inside drawHomeScree: %s\n", buf);

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

void UITask::drawWifiNewScreen() {
    // PLACEHOLDER!!!! for entering new wifi info
    display->text("Placeholder for", 0, 20);
    display->text("new", 0, 32);
    display->text("Press to cont.", 0, 50);
}
