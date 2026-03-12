//
// Created by Keijo Länsikunnas on 12.2.2024.
//

#ifndef UART_IRQ_IPSTACK_H
#define UART_IRQ_IPSTACK_H

#include <cstdint>
#include <memory>
#include <string>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"

#include "FreeRTOS.h"
#include "task.h"

#define DEFAULT_HTTP_SERVER "54.158.231.69"

class IPStack {
public:
    IPStack(const char *ssid, const char *pw, const uint8_t *tls_cert, size_t tls_cert_len);

    int connect(const char *hostname, int port);
    int connect(uint32_t hostname, int port);
    int read(unsigned char *buffer, int len, int timeout);
    int write(unsigned char *buffer, int len, int timeout);
    int disconnect();

    static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *arg);

    // lwip callback functions
    static err_t tcp_client_sent(void *arg, struct altcp_pcb *tpcb, u16_t len);
    static err_t tcp_client_poll(void *arg, struct altcp_pcb *tpcb) ;
    static void tcp_client_err(void *arg, err_t err);
    static err_t tcp_client_recv(void *arg, struct altcp_pcb *tpcb, struct pbuf *p, err_t err);
    static err_t tcp_client_connected(void *arg, struct altcp_pcb *tpcb, err_t err);

    static const int BUF_SIZE{2048};
    static const int POLL_TIME_S{5};

    bool operator()();
private:
    struct altcp_tls_config *tls_config;
    struct altcp_pcb *tcp_pcb;
    ip_addr_t remote_addr;
    uint8_t buffer[BUF_SIZE];
    uint16_t count;
    uint32_t dropped;
    uint16_t wr; // write index
    uint16_t rd; // read index
    bool connected;
    bool wifi_connected;

    std::string http_server;
    bool dns_ready = false;
};


#endif //UART_IRQ_IPSTACK_H
