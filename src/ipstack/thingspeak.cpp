#include "thingspeak.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include <iostream>
#include <sstream>
#include <random>
#include <string.h>
#include "controller/ControllerEnums.h"
#include "hardware/gpio.h"

#define LED_1 20
#define LED_2 21
#define LED_3 22

ThingSpeak::ThingSpeak(QueueHandle_t _cloud_q, QueueHandle_t _controller_q, QueueHandle_t _wifi_q, EventGroupHandle_t eventGroup)
: cloud_q(_cloud_q), controller_q(_controller_q), wifi_q(_wifi_q), eventGroup(eventGroup)
{
    ipstack_mtx = xSemaphoreCreateMutex();

    xTaskCreate(ThingSpeak::connect_task, "CONNECT", 4096,
        static_cast<void*>(this),
        tskIDLE_PRIORITY + 2,
        &connect_task_handle
    );

    xTaskCreate(ThingSpeak::send_task, "SEND", 2048,
        static_cast<void *>(this), 
        tskIDLE_PRIORITY + 1, 
        &send_task_handle
    );
    xTaskCreate(ThingSpeak::read_task, "READ", 2048, 
        static_cast<void *>(this), 
        tskIDLE_PRIORITY + 1, 
        &read_task_handle
    );
}

void ThingSpeak::connect_task(void *param)
{
    ThingSpeak *ts = static_cast<ThingSpeak*>(param);
    
    // Resolve Conflict: Using dynamic Wi-Fi info from the queue 
    // while maintaining the TLS certificate from the encryption branch
    const uint8_t tls_cert[] = THINGSPEAK_CERT;
    wifi_config_t wifiInfo;

    if (xQueueReceive(ts->wifi_q, &wifiInfo, portMAX_DELAY) == pdPASS) {
        // Initialize IPStack with dynamic credentials and TLS cert
        ts->ipstack = std::make_shared<IPStack>(wifiInfo.ssid, wifiInfo.pwd, tls_cert, sizeof(tls_cert));
    }

    // Attempt to bring up the interface
    if ((*ts->ipstack)()) {
        xEventGroupSetBits(ts->eventGroup, EVENT_BIT_0);
        printf("Interface UP: setting bit 0.\n");
    }
    else {
        xEventGroupSetBits(ts->eventGroup, EVENT_BIT_1);
        printf("Interface DOWN: setting bit 1. SUSPENDING CLOUD TASK.\n");
        vTaskSuspend(NULL);
    }

    // Hardware feedback
    gpio_init(LED_1);
    gpio_set_dir(LED_1, GPIO_OUT);
    gpio_put(LED_1, (*ts->ipstack)());

    // Wake up sub-tasks
    xTaskNotifyGive(ts->read_task_handle);
    xTaskNotifyGive(ts->send_task_handle);
    
    // Kill this setup task
    vTaskDelete(NULL); 
}


void ThingSpeak::send_task(void *param)
{
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    ThingSpeak *ts = static_cast<ThingSpeak*>(param);
    auto ipstack = ts->ipstack;

    gpio_init(LED_2);
    gpio_set_dir(LED_2, GPIO_OUT);

    char http_msg[512];
    sensorData data;
    unsigned char *buffer = new unsigned char[RESULT_BUF_SIZE];

    while (true) {
        if (xQueueReceive(ts->cloud_q, &data, portMAX_DELAY)) {
            xSemaphoreTake(ts->ipstack_mtx, portMAX_DELAY);
            int rc = ipstack->connect(HTTP_SERVER_HOSTNAME, HTTP_PORT);
            if (rc == 0) {
                gpio_put(LED_2, true);
                std::ostringstream http_body_ss;
                http_body_ss << "api_key=" << THINGSPEAK_WRITE_API_KEY
                    << "&field" << data.co2.index << "=" << data.co2.value
                    << "&field" << data.rh.index << "=" << data.rh.value
                    << "&field" << data.temp.index << "=" << data.temp.value
                    << "&field" << data.fan.index << "=" << data.fan.value
                    << "&field" << data.co2Set.index << "=" << data.co2Set.value;
                auto http_body = http_body_ss.str();
    
                snprintf(http_msg, sizeof(http_msg), SEND_DATA_REQ, http_body.size(), http_body.c_str());
                printf("Sending: %s\n", http_msg);
    
                rc = ipstack->write((unsigned char*)http_msg, strlen(http_msg), 1000);
                if (rc > 0) {
                    auto rv = ipstack->read(buffer, RESULT_BUF_SIZE, 2000);
                    buffer[rv] = 0;
                    printf("rv=%d\n%s\n", rv, buffer);
                } else {
                    printf("Fail\n");
                }
                ipstack->disconnect();
            }
            xSemaphoreGive(ts->ipstack_mtx);
        }
        gpio_put(LED_2, false);
    }
}

bool ThingSpeak::parse_talkback_response_json(const char *response, int *co2_set_point)
{
    jsmn_parser parser;
    jsmn_init(&parser);

    std::string response_str = response;

    std::string status = "";
    size_t status_prefix_pos = response_str.find("Status: "); 
    if (status_prefix_pos == std::string::npos) return false;

    size_t start_index = status_prefix_pos + 8;
    size_t end_index = response_str.find('\n', start_index);
    status = response_str.substr(start_index, end_index - start_index);

    if (!status.empty() && status.back() == '\r') {
        status.pop_back();
    } else {
        return false;
    }

    if (status != "200 OK") return false;

    std::string json = "";
    size_t json_start = response_str.find("{"); 
    size_t json_end = response_str.rfind("}");
    if (json_start == std::string::npos || json_end == std::string::npos) return false;

    json = response_str.substr(json_start, json_end - json_start + 1);
    jsmntok_t tokens[JSMN_TOKENS_SIZE];
    int r = jsmn_parse(&parser, json.c_str(), json.size(), tokens, JSMN_TOKENS_SIZE);
    if (r < 0) return false;

    for (int i = 0; i < JSMN_TOKENS_SIZE; i++) {
        if (tokens[i].type == JSMN_STRING) {
            std::string json_val = json.substr(tokens[i].start, tokens[i].end - tokens[i].start);
            char command[] = "SET_CO2|";
            if (json_val.find(command) != std::string::npos) {
                std::stringstream co2_set_point_ss(json_val.substr(strlen(command)));
                int n;
                if (co2_set_point_ss >> n) {
                    *co2_set_point = n;
                    return true;
                } else {
                    return false;
                }
            }
        }
    }
    return false;
}

void ThingSpeak::read_task(void *param)
{
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    ThingSpeak *ts = static_cast<ThingSpeak*>(param);
    auto ipstack = ts->ipstack;

    gpio_init(LED_3);
    gpio_set_dir(LED_3, GPIO_OUT);
    
    char http_msg[512];
    std::ostringstream http_body_ss;
    http_body_ss << "api_key=" << THINGSPEAK_TALKBACK_API_KEY;
    auto http_body = http_body_ss.str();
    snprintf(http_msg, sizeof(http_msg), TALKBACK_REQ,
        THINGSPEAK_TALKBACK_ID, http_body.size(), http_body.c_str());
    
    unsigned char *result_buffer = new unsigned char[RESULT_BUF_SIZE];

    while (true) {
        xSemaphoreTake(ts->ipstack_mtx, portMAX_DELAY);
        int rc = ipstack->connect(HTTP_SERVER_HOSTNAME, HTTP_PORT);
        if (rc == 0) {
            gpio_put(LED_3, true);
            
            //printf("Sending: %s\n", http_msg);
            rc = ipstack->write((unsigned char*)http_msg, strlen(http_msg), 1000);
            if (rc > 0) {
                auto rv = ipstack->read(result_buffer, RESULT_BUF_SIZE, 2000);
                result_buffer[rv] = 0;
                int co2_set_point;
                if (ts->parse_talkback_response_json((const char*)result_buffer, &co2_set_point) && (co2_set_point <= 1500 && co2_set_point >= 0)) {
                    if (xQueueSendToBack(ts->controller_q, &co2_set_point, portMAX_DELAY) == pdTRUE) {
                        std::cout << "NEW CO2_SET_POINT: " << co2_set_point << std::endl;
                    } else {
                        std::cout << "ERROR SETTING CO2_SET_POINT" << std::endl;
                    }
                } else {
                    std::cout << "NO DATA IN TALKBACK" << std::endl;
                }
            }
            ipstack->disconnect();
        }
        gpio_put(LED_3, false);

        xSemaphoreGive(ts->ipstack_mtx);
        vTaskDelay(pdMS_TO_TICKS(20000));
    }
}
