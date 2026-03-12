//
// Created by renek on 04/03/2026.
//

#ifndef UITASK_H
#define UITASK_H

#include <memory>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "ssd1306os.h"
#include "controller/ControllerEnums.h"
#include "RotaryEncoder.h"
#include "ConfigStorage.h"
#include "event_groups.h"

class UITask {
public:
    UITask(QueueHandle_t displayQueue, QueueHandle_t controllerQueue, QueueHandle_t wifiQueue, EventGroupHandle_t eventGroup, std::shared_ptr<ConfigStorage> storage);

private:
    static void runner(void *params);
    void run();

    void drawHomeScreen();
    void drawSetpointScreen();
    void drawSetpointSelectScreen();
    void drawSetpointSavedScreen();
    void drawWifiMenuScreen();
    void drawWifiSavedScreen();
    void drawWifiNewSSIDScreen();
    void drawWifiNewPwdScreen();
    void drawWifiConfirmScreen();
    void drawWifiConnecting();
    void drawWifiConnectionError();
    void handleInput();
    void checkTimeOut();
    void checkEventBits();

    void displayWifiSetInfo();
    void saveInfoToMemory();

    // display is created inside run() after the scheduler starts
    std::shared_ptr<ssd1306os> display;
    std::shared_ptr<ConfigStorage> storage;
    QueueHandle_t displayQueue;
    QueueHandle_t controllerQueue;
    QueueHandle_t wifiQueue;
    EventGroupHandle_t eventGroup;
    TaskHandle_t handle;

    RotaryEncoder encoder;

    enum class Screen { WIFI_MENU, WIFI_SAVED, WIFI_ERROR, WIFI_CONNECTING, WIFI_NEW_SSID, WIFI_NEW_PWD, WIFI_CONFIRM, SETPOINT_SELECT, SETPOINT_SAVED, HOME, EDIT_SETPOINT };
    Screen currentScreen{Screen::WIFI_MENU};
    int wifiMenuSelection{0};
    int spMenuSelection{0};

    int editValue{1000};
    int currentSetpoint{1000};

    char currentChar{'A'};
    std::string wifiInfo;
    TickType_t lastActive{0};
    wifi_config_t wifiConfigInfo{.ssid = " ", .pwd = " "};
    bool setWifiPwd{false};

    sensorData latestData{};   // cached data received from displayQueue
};

#endif // UITASK_H
