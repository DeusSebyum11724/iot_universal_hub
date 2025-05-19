#include "ssdp_discovery.h"
#include "device_control.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "ssdp_discovery";
#define SSDP_ADDR "239.255.255.250"
#define SSDP_PORT 1900

static void add_ssdp_device(const char* usn, const char* st, const char* location) {
    device_t dev = {0};
    snprintf(dev.id, sizeof(dev.id), "ssdp_%s", usn);
    snprintf(dev.name, sizeof(dev.name), "%s", st);
    // Heuristic: tip device după ST (search target)
    if (strstr(st, "MediaRenderer") || strstr(st, "MediaServer")) {
        strcpy(dev.type, "media");
    } else if (strstr(st, "tv") || strstr(st, "Television")) {
        strcpy(dev.type, "tv");
    } else if (strstr(st, "Lighting")) {
        strcpy(dev.type, "light");
    } else {
        strcpy(dev.type, "unknown");
    }
    strcpy(dev.proto, "ssdp");
    strcpy(dev.status, "unavailable");
    snprintf(dev.extra, sizeof(dev.extra), "location=%s", location ? location : "n/a");
    device_add(&dev);
}

static void ssdp_scan_task(void *arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed");
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(SSDP_ADDR);
    dest.sin_port = htons(SSDP_PORT);

    char msg[] =
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "MAN: \"ssdp:discover\"\r\n"
        "MX: 2\r\n"
        "ST: ssdp:all\r\n\r\n";

    char recvbuf[1536];
    struct timeval timeout = { .tv_sec = 2, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while (1) {
        sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&dest, sizeof(dest));
        ESP_LOGI(TAG, "Sent SSDP M-SEARCH");

        for (int i = 0; i < 20; ++i) { // primește timp de 2 secunde
            struct sockaddr_in from;
            socklen_t fromlen = sizeof(from);
            int r = recvfrom(sock, recvbuf, sizeof(recvbuf) - 1, 0, (struct sockaddr *)&from, &fromlen);
            if (r > 0) {
                recvbuf[r] = 0;
                // caută USN, ST, LOCATION în răspuns
                char *usn = strstr(recvbuf, "USN:");
                char *st = strstr(recvbuf, "ST:");
                char *location = strstr(recvbuf, "LOCATION:");
                char usn_val[128] = "", st_val[128] = "", loc_val[256] = "";
                if (usn) sscanf(usn, "USN: %127[^\r\n]", usn_val);
                if (st) sscanf(st, "ST: %127[^\r\n]", st_val);
                if (location) sscanf(location, "LOCATION: %255[^\r\n]", loc_val);
                if (usn_val[0] && st_val[0]) {
                    ESP_LOGI(TAG, "SSDP found: %s [%s]", usn_val, st_val);
                    add_ssdp_device(usn_val, st_val, loc_val);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(15000)); // la fiecare 15 secunde
    }
}

void ssdp_discovery_init(void) {
    xTaskCreate(ssdp_scan_task, "ssdp_scan_task", 6144, NULL, 5, NULL);
    ESP_LOGI(TAG, "SSDP discovery started.");
}
