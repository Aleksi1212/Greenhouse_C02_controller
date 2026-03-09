#ifndef THINGSPEAK_H
#define THINGSPEAK_H

#include "network_info.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "IPStack.h"
#include <vector>
#include <string>
// struct CloudData_t {
//     float co2_level;
//     float rh;
//     float temp;
//     float vent_fan_speed;
//     float co2_set_point;
// };
enum FieldID {
    CO2_LEVEL = 1,
    REALATIVE_HUMIDITY,
    TEMPERATURE,
    VENT_FAN_SPEED,
    CO2_SET_POINT,
    // NUM_OF_FIELDS
};

enum ERROR_CODES {
    OK         = 0,
    MEM        = -1,
    BUF        = -2,
    TIMEOUT    = -3,
    RTE        = -4,
    INPROGRESS = -5,
    VAL        = -6,
    WOULDBLOCK = -7,
    USE        = -8,
    ALREADY    = -9,
    ISCONN     = -10,
    CONN       = -11,
    IF         = -12,
    ABRT       = -13,
    RST        = -14,
    CLSD       = -15,
    ARG        = -16
};

struct CloudData_t {
    FieldID field_id;
    float field_data;
};

/*
    Some of these addresses is the thingspeak server ???
*/
#define HTTP_SERVER_HOSTNAME "api.thingspeak.com"
#define DEFAULT_HTTP_SERVER "54.158.231.69"
#define HTTP_PORT 80

#define SEND_DATA_REQ "POST /update.json HTTP/1.1\r\n" \
                    "Host: api.thingspeak.com\r\n" \
                    "User-Agent: PicoW\r\n" \
                    "Content-Type: application/x-www-form-urlencoded\r\n" \
                    "Content-Length: %d\r\n" \
                    "Accept: */*\r\n" \
                    "\r\n" \
                    "%s" // HTTP body: api_key=key&field<id>=data...

#define CLOUD_Q_SIZE 10
#define RESULT_BUF_SIZE 2048

class ThingSpeak {
private:
    std::shared_ptr<IPStack> ipstack;

    QueueHandle_t cloud_q;
    QueueHandle_t controller_q;

    TaskHandle_t connect_task_handle;
    TaskHandle_t send_task_handle;
    TaskHandle_t read_task_handle;

    std::string http_server;
    bool dns_ready = false;

    static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
    static void connect_task(void *param);

    static void send_task(void *param);
    static void read_task(void *param);

    static void test_task(void *param);
public:
    ThingSpeak(QueueHandle_t _cloud_q, QueueHandle_t _controller_q);
    bool send_to_queue(std::vector<CloudData_t*> data, TickType_t ticksToWait);
    bool send_to_queue(CloudData_t *data, TickType_t ticksToWait);
};



#endif