#ifndef THINGSPEAK_H
#define THINGSPEAK_H

#include "network_info.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "IPStack.h"

struct CloudData_t {
    float co2_level;
    float rh;
    float temp;
    float vent_fan_speed;
    float co2_set_point;
};

#define HTTP_SERVER "api.thingspeak.com"

class ThingSpeak : private IPStack {
private:
    QueueHandle_t cloud_q;
    QueueHandle_t controller_q;

    int connect_rc = -1;
    int send_rc = -1;
    int read_rc = -1;

    static void send_task(void *param);
    static void read_task(void *param);

public:
    ThingSpeak(/*QueueHandle_t _controller_q*/);
    void send_to_queue(CloudData_t data, TickType_t ticksToWait);
};



#endif