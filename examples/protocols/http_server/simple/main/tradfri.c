#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "esp_log.h"

#define TAG "tradfri"

bool find_tradfri_gateway(char *ip_out) {
    const char *msearch =
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "MAN: \"ssdp:discover\"\r\n"
        "MX: 2\r\n"
        "ST: ssdp:all\r\n\r\n";

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;

    struct sockaddr_in dest_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(1900)
    };
    inet_pton(AF_INET, "239.255.255.250", &dest_addr.sin_addr);

    struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int sent = sendto(sock, msearch, strlen(msearch), 0,
                      (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (sent < 0) {
        close(sock);
        return false;
    }

    char recv_buf[1024];
    struct sockaddr_in source_addr;
    socklen_t addr_len = sizeof(source_addr);
    int found = 0;

    while (recvfrom(sock, recv_buf, sizeof(recv_buf) - 1, 0,
                    (struct sockaddr*)&source_addr, &addr_len) > 0) {
        recv_buf[1023] = '\0';
        if (strstr(recv_buf, "gws/1.0") || strstr(recv_buf, "IKEA")) {
            inet_ntop(AF_INET, &source_addr.sin_addr, ip_out, 64);
            ESP_LOGI(TAG, "Tradfri detectat la %s", ip_out);
            found = 1;
            break;
        }
    }

    close(sock);
    return found;
}
