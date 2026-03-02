#include "thingspeak.h"
#include <iostream>

ThingSpeak::ThingSpeak(/*QueueHandle_t _controller_q*/) :
    IPStack(SSID, PW)
    // controller_q(_controller_q)
{
    cloud_q = xQueueCreate(CLOUD_Q_SIZE, sizeof(CloudData_t));
    connect_rc = connect(HTTP_SERVER, 80);

    xTaskCreate(ThingSpeak::send_task, "SEND", 512, 
        static_cast<void *>(this), 
        tskIDLE_PRIORITY + 1, 
        send_task_handle
    );
}

void ThingSpeak::send_task(void *param)
{
    ThingSpeak *ts = static_cast<ThingSpeak*>(param);

    std::string http_fields = "";
    char http_msg[512];
    CloudData_t data;

    while (true) {
        eTaskState task_state = eTaskGetState(ts->send_task_handle);

        if (ts->connect_rc == 0) {
            if (task_state == eSuspended) vTaskResume(ts->send_task_handle);

            if (xQueueReceive(cloud_q, &data, portMAX_DELAY)) {
                std::string http_field = "field" + data.field_id + "=" + data.data + "&";
                http_fields += http_field;
                continue;
            }
            if (http_fields != "") {
                snprintf(http_msg, 512, SEND_DATA_REQ, THINGSPEAK_WRITE_API_KEY, http_fields.c_str());
                ts->write(http_msg, 512, 60);
            }
            http_fields = "";
        } else if (ts->connect_rc != 0 && task_state != eSuspended) {
            vTaskSuspend(NULL)
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

bool ThingSpeak::send_to_queue(std::vector<CloudData_t*> data, TickType_t ticksToWait)
{
    int count = 0;
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (count == CLOUD_Q_SIZE) break;
        bool fail xQueueSendToBack(cloud_q, *it, ticksToWait) != pdPASS;
        if (fail) return false;
    }
    return true;
}
bool ThingSpeak::send_to_queue(CloudData_t *data, TickType_t ticksToWait)
{
    return xQueueSendToBack(cloud_q, data, ticksToWait) == pdPASS;
}