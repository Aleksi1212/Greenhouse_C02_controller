#include "thingspeak.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include <iostream>
#include <sstream>
#include <random>
#include <string.h>
#include "controller/ControllerEnums.h"

/*
    FOR TESTING
*/
int randomInt(int min, int max) {
    static std::mt19937 rng(std::random_device{}()); // seeded once
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}
float randomFloat(float min, float max) {
    static std::mt19937 rng(std::random_device{}()); // seeded once
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

void ThingSpeak::test_task(void *param)
{
    ThingSpeak *ts = static_cast<ThingSpeak*>(param);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    while (true) {
        CloudData_t data = { .field_id = (FieldID)randomInt(1, 5), .field_data = randomFloat(0.0f, 100.0f) };
        bool ok = ts->send_to_queue(&data, pdMS_TO_TICKS(100));
        printf("%s\n", ok ? "OK" : "FAIL");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
/*
    END
*/

ThingSpeak::ThingSpeak(QueueHandle_t _cloud_q, QueueHandle_t _controller_q)
: http_server(DEFAULT_HTTP_SERVER), cloud_q(_cloud_q), controller_q(_controller_q)
{
    xTaskCreate(ThingSpeak::connect_task, "CONNECT", 4096, 
        static_cast<void*>(this),
        tskIDLE_PRIORITY + 2,
        &connect_task_handle
    );

    xTaskCreate(ThingSpeak::send_task, "SEND", 4096, 
        static_cast<void *>(this), 
        tskIDLE_PRIORITY + 1, 
        &send_task_handle
    );
    // xTaskCreate(ThingSpeak::test_task, "TEST", 2048, 
    //     static_cast<void *>(this), 
    //     tskIDLE_PRIORITY + 1, 
    //     &read_task_handle
    // );
}

void ThingSpeak::dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    ThingSpeak *ts = static_cast<ThingSpeak*>(callback_arg);
    if (ipaddr) {
        ts->http_server = ipaddr_ntoa(ipaddr);
        printf("DNS resolved %s to %s\n", name, ipaddr_ntoa(ipaddr));
    } else {
        ts->http_server = DEFAULT_HTTP_SERVER;
        printf("DNS resolution failed for %s\n", name);
    }
    ts->dns_ready = true;
}

void ThingSpeak::connect_task(void *param)
{
    ThingSpeak *ts = static_cast<ThingSpeak*>(param);

    ts->ipstack = std::make_shared<IPStack>(SSID, PW);

    ip_addr_t addr;
    int err = (int)dns_gethostbyname(HTTP_SERVER_HOSTNAME, &addr,
        ThingSpeak::dns_callback, param);

    while (!ts->dns_ready) vTaskDelay(pdMS_TO_TICKS(100));

    //xTaskNotifyGive(ts->read_task_handle);
    xTaskNotifyGive(ts->send_task_handle);
    vTaskSuspend(NULL);
}


void ThingSpeak::send_task(void *param)
{
    ThingSpeak *ts = static_cast<ThingSpeak*>(param);

    char http_msg[512];
    sensorData data;
    unsigned char *buffer = new unsigned char[RESULT_BUF_SIZE];

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    while (true) {
        if (xQueueReceive(ts->cloud_q, &data, pdMS_TO_TICKS(5000))
            && ts->ipstack->connect(ts->http_server.c_str(), HTTP_PORT) == 0)
        {
            // std::ostringstream http_body_ss;
            // http_body_ss << "api_key="
            //     << THINGSPEAK_WRITE_API_KEY
            //     << "&field"
            //     << data.field_id
            //     << "="
            //     << data.field_data;
            // auto http_body = http_body_ss.str();
            std::ostringstream http_body_ss;
            http_body_ss << "api_key="
                << THINGSPEAK_WRITE_API_KEY
                << "&field1="
                << data.co2
                << "&field2="
                << data.rh
                << "&field3="
                << data.temp;
            auto http_body = http_body_ss.str();

            snprintf(http_msg, sizeof(http_msg), SEND_DATA_REQ, http_body.size(), http_body.c_str());
            printf("Sending: %s\n", http_msg);

            int rc = ts->ipstack->write((unsigned char*)http_msg, strlen(http_msg), 1000);
            if (rc > 0) {
                auto rv = ts->ipstack->read(buffer, RESULT_BUF_SIZE, 2000);
                buffer[rv] = 0;
                printf("rv=%d\n%s\n", rv, buffer);
            } else {
                printf("Fail\n");
            }
            ts->ipstack->disconnect();
        }
    }
}

bool ThingSpeak::send_to_queue(std::vector<CloudData_t*> data, TickType_t ticksToWait)
{
    int count = 0;
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (count == CLOUD_Q_SIZE) break;
        bool fail = xQueueSendToBack(cloud_q, *it, ticksToWait) != pdPASS;
        if (fail) return false;
    }
    return true;
}
bool ThingSpeak::send_to_queue(CloudData_t *data, TickType_t ticksToWait)
{
    return xQueueSendToBack(cloud_q, data, ticksToWait) == pdPASS;
}