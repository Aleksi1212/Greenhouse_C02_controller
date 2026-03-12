#ifndef THINGSPEAK_H
#define THINGSPEAK_H

#include "network_info.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "IPStack.h"
#include "queue.h"
#include <vector>
#include <string>
#include "semphr.h"

#define JSMN_STATIC
#include "event_groups.h"
#include "jsmn.h"

/*
    Some of these addresses is the thingspeak server ???
*/
#define HTTP_SERVER_HOSTNAME "api.thingspeak.com"
#define HTTP_PORT 443

#define SEND_DATA_REQ "POST /update.json HTTP/1.1\r\n" \
                    "Host: api.thingspeak.com\r\n" \
                    "User-Agent: PicoW\r\n" \
                    "Content-Type: application/x-www-form-urlencoded\r\n" \
                    "Content-Length: %d\r\n" \
                    "Accept: */*\r\n" \
                    "\r\n" \
                    "%s" // HTTP body: api_key=key&field<id>=data...

#define TALKBACK_REQ "POST /talkbacks/%d/commands/execute.json HTTP/1.1\r\n" \
                    "Host: api.thingspeak.com\r\n" \
                    "Content-Type: application/x-www-form-urlencoded\r\n" \
                    "Content-Length: %d\r\n" \
                    "Accept: */*\r\n" \
                    "\r\n" \
                    "%s" // HTTP body: api_key=key

#define THINGSPEAK_CERT "-----BEGIN CERTIFICATE-----\n\
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n\
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n\
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n\
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n\
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n\
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n\
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n\
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n\
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n\
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n\
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n\
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n\
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n\
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n\
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n\
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n\
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n\
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n\
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n\
MrY=\n\
-----END CERTIFICATE-----\n"

#define CLOUD_Q_SIZE 10
#define RESULT_BUF_SIZE 2048
#define JSMN_TOKENS_SIZE 20


class ThingSpeak {
private:
    std::shared_ptr<IPStack> ipstack;

    QueueHandle_t cloud_q;
    QueueHandle_t controller_q;
    QueueHandle_t wifi_q;

    TaskHandle_t connect_task_handle;
    TaskHandle_t send_task_handle;
    TaskHandle_t read_task_handle;

    SemaphoreHandle_t ipstack_mtx;

    EventGroupHandle_t eventGroup;

    static void connect_task(void *param);

    static void send_task(void *param);
    static void read_task(void *param);

    bool parse_talkback_response_json(const char *response, int *co2_set_point);
public:
    ThingSpeak(QueueHandle_t _cloud_q, QueueHandle_t _controller_q, QueueHandle_t _wifi_q, EventGroupHandle_t eventGroup);
};



#endif