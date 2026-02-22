#include "thingspeak.h"
#include <iostream>

ThingSpeak::ThingSpeak(/*QueueHandle_t _controller_q*/) :
    IPStack(SSID, PW)
    // controller_q(_controller_q)
{
    cloud_q = xQueueCreate(10, sizeof(CloudData_t));
    connect_rc = connect(HTTP_SERVER, 80);

    xTaskCreate(ThingSpeak::send_task, "SEND", 512, static_cast<void *>(this), tskIDLE_PRIORITY + 1, NULL);
}

void ThingSpeak::send_task(void *param)
{
    ThingSpeak *ts = static_cast<ThingSpeak*>(param);

    while (true) {
        if (ts->connect_rc == 0) {
            std::cout << "connected" << std::endl;
        } else {
            std::cout << "not connected" << std::endl;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}