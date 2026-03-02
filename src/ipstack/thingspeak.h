#ifndef THINGSPEAK_H
#define THINGSPEAK_H

#include "network_info.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "IPStack.h"
#include <vector>

// struct CloudData_t {
//     float co2_level;
//     float rh;
//     float temp;
//     float vent_fan_speed;
//     float co2_set_point;
// };
enum FieldID {
    CO2_LEVEL,
    REALATIVE_HUMIDITY,
    TEMPERATURE,
    VENT_FAN_SPEED,
    CO2_SET_POINT,
    // NUM_OF_FIELDS
}

struct CloudData_t {
    FieldID field_id;
    float data;
};

/*
    Some of these addresses is the thingspeak server ???
*/
#define HTTP_SERVER "18.215.51.65"
// #define HTTP_SERVER "54.158.231.69"
// #define HTTP_SERVER "api.thingspeak.com"

#define SEND_DATA_REQ \
    "POST /update.json HTTP/1.1\r\n" \
    "Host: api.thingspeak.com\r\n" \
    "User-Agent: PicoW\r\n" \
    "Accept: */*\r\n" \
    "Content-Length: 256\r\n" \
    "Content-Type: application/x-www-form-urlencoded\r\n" \
    "api_key=%s&%s"

#define CLOUD_Q_SIZE 10

class ThingSpeak : private IPStack {
private:
    QueueHandle_t cloud_q;
    QueueHandle_t controller_q;

    TaskHandle_t send_task_handle;
    TaskHandle_t read_task_handle;

    int connect_rc = -1;
    int send_rc = -1;
    int read_rc = -1;

    static void send_task(void *param);
    static void read_task(void *param);

public:
    ThingSpeak(/*QueueHandle_t _controller_q*/);
    bool send_to_queue(std::vector<CloudData_t*> data, TickType_t ticksToWait);
    bool send_to_queue(CloudData_t *data, TickType_t ticksToWait);
};



#endif