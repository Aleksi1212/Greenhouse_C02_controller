#include "thingspeak.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include <iostream>
#include <sstream>
#include <random>
#include <string.h>

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
/*
    END
*/

ThingSpeak::ThingSpeak() : http_server(DEFAULT_HTTP_SERVER)
{
    cloud_q = xQueueCreate(CLOUD_Q_SIZE, sizeof(CloudData_t));

    xTaskCreate(ThingSpeak::connect_task, "CONNECT", 2048, 
        static_cast<void*>(this),
        tskIDLE_PRIORITY + 2,
        &connect_task_handle
    );

    xTaskCreate(ThingSpeak::send_task, "SEND", 1024, 
        static_cast<void *>(this), 
        tskIDLE_PRIORITY + 1, 
        &send_task_handle
    );
    xTaskCreate(ThingSpeak::test_task, "TEST", 512, 
        static_cast<void *>(this), 
        tskIDLE_PRIORITY + 1, 
        NULL
    );
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
    ts->connect_rc = ts->ipstack->connect(ts->http_server, HTTP_PORT);
}

void ThingSpeak::connect_task(void *param)
{
    ThingSpeak *ts = static_cast<ThingSpeak*>(param);
    ts->ipstack = std::make_shared<IPStack>(SSID, PW);

    ip_addr_t addr;
    dns_gethostbyname("api.thingspeak.com", &addr, ThingSpeak::dns_callback, param);
    xTaskNotifyGive(ts->send_task_handle);
    // xTaskNotifyGive(read_task_handle);

    while (true) {
        if (ts->connect_rc != 0) {
            printf("Reconnect to %s\n", ts->http_server);
            ts->connect_rc = ts->ipstack->connect(ts->http_server, HTTP_PORT);
        } else {
            auto tasks = &ts->suspended_tasks;
            int task_count = tasks->size();
            if (task_count > 0) {
                printf("Restarting %d network tasks\n", task_count);
                for (auto it = tasks->begin(); it != tasks->end();) {
                    vTaskResume(*it);
                    it = tasks->erase(it);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void ThingSpeak::test_task(void *param)
{
    ThingSpeak *ts = static_cast<ThingSpeak*>(param);

    while (true) {
        CloudData_t data = { .field_id = (FieldID)randomInt(1, 8), .field_data = randomFloat(0.0f, 100.0f) };
        bool ok = ts->send_to_queue(&data, pdMS_TO_TICKS(100));
        printf("%s\n", ok ? "OK" : "FAIL");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void ThingSpeak::send_task(void *param)
{
    ThingSpeak *ts = static_cast<ThingSpeak*>(param);

    char http_msg[512];
    CloudData_t data;

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    while (true) {
        if (ts->connect_rc != 0) {
            ts->suspended_tasks.push_back(ts->send_task_handle);
            vTaskSuspend(NULL);
        } else if (ts->connect_rc == 0 && xQueueReceive(ts->cloud_q, &data, portMAX_DELAY)) {
            std::ostringstream field;
            field << "field" << data.field_id << "=" << data.field_data;
            snprintf(http_msg, sizeof(http_msg), SEND_DATA_REQ, THINGSPEAK_WRITE_API_KEY, field.str().c_str());
            printf("Sending: %s\n", http_msg);
            ts->ipstack->write((unsigned char*)http_msg, strlen(http_msg), 1000);
        }

        // if (ts->connect_rc == 0) {
        //     if (task_state == eSuspended) vTaskResume(ts->send_task_handle);

        //     if (xQueueReceive(cloud_q, &data, portMAX_DELAY)) {
        //         std::string http_field = "field" + data.field_id + "=" + data.data + "&";
        //         http_fields += http_field;
        //         continue;
        //     }
        //     if (http_fields != "") {
        //         snprintf(http_msg, 512, SEND_DATA_REQ, THINGSPEAK_WRITE_API_KEY, http_fields.c_str());
        //         ts->write(http_msg, 512, 60);
        //     }
        //     http_fields = "";
        // } else if (ts->connect_rc != 0 && task_state != eSuspended) {
        //     vTaskSuspend(NULL)
        // }
        // vTaskDelay(pdMS_TO_TICKS(1000));
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