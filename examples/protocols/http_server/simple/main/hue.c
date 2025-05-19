#include "hue.h"
#include "http_utils.h"
#include "esp_log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define TAG "hue"
#define RESPONSE_SIZE 4096

bool hue_pair(const char* ip, char* user_out) {
    const char* post_data = "{\"devicetype\":\"esp32_hub#nasu\"}";
    char http_req[512];
    snprintf(http_req, sizeof(http_req),
        "POST /api HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n"
        "%s", ip, (int)strlen(post_data), post_data);

    char* response = malloc(RESPONSE_SIZE);
    if (!response) return false;

    int len = http_raw_request(ip, 80, http_req, response, RESPONSE_SIZE);
    if (len <= 0 || strncmp(response, "HTTP/1.1 2", 10) != 0) {
        free(response);
        return false;
    }

    char* json_start = strchr(response, '[');
    if (!json_start) {
        free(response);
        return false;
    }

    char* usr = strstr(json_start, "\"username\":\"");
    if (usr) {
        usr += strlen("\"username\":\"");
        char* end = strchr(usr, '"');
        if (end) {
            size_t len = end - usr;
            if (len >= 64) len = 63;
            strncpy(user_out, usr, len);
            user_out[len] = '\0';
            free(response);
            return true;
        }
    }

    free(response);
    return false;
}

bool hue_get_devices(const char* ip, const char* user, char* json_out, size_t max_len) {
    char http_req[256];
    snprintf(http_req, sizeof(http_req),
        "GET /api/%s/lights HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n", user, ip);

    char* response = malloc(RESPONSE_SIZE);
    if (!response) return false;

    int len = http_raw_request(ip, 80, http_req, response, RESPONSE_SIZE);
    if (len <= 0 || strncmp(response, "HTTP/1.1 2", 10) != 0) {
        free(response);
        return false;
    }

    char* json_start = strchr(response, '{');
    if (!json_start) {
        free(response);
        return false;
    }

    strncpy(json_out, json_start, max_len - 1);
    json_out[max_len - 1] = '\0';
    free(response);
    return true;
}

bool hue_set_state(const char* ip, const char* user, const char* device_id, const char* state_json) {
    char http_req[1024];
    snprintf(http_req, sizeof(http_req),
        "PUT /api/%s/lights/%s/state HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        user, device_id, ip, (int)strlen(state_json), state_json);

    char* response = malloc(RESPONSE_SIZE);
    if (!response) return false;

    int len = http_raw_request(ip, 80, http_req, response, RESPONSE_SIZE);
    free(response);
    return (len > 0);
}

bool hue_get_info(const char* ip, const char* user, char* json_out, size_t max_len) {
    char http_req[256];
    snprintf(http_req, sizeof(http_req),
        "GET /api/%s/config HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n", user, ip);

    char* response = malloc(RESPONSE_SIZE);
    if (!response) return false;

    int len = http_raw_request(ip, 80, http_req, response, RESPONSE_SIZE);
    if (len <= 0 || strncmp(response, "HTTP/1.1 2", 10) != 0) {
        free(response);
        return false;
    }

    char* json_start = strchr(response, '{');
    if (!json_start) {
        free(response);
        return false;
    }

    strncpy(json_out, json_start, max_len - 1);
    json_out[max_len - 1] = '\0';
    free(response);
    return true;
}

bool hue_discover(char* ip_out) {
    const char *msearch =
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "MAN: \"ssdp:discover\"\r\n"
        "MX: 2\r\n"
        "ST: upnp:rootdevice\r\n\r\n";

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
        if (strstr(recv_buf, "IpBridge")) {
            inet_ntop(AF_INET, &source_addr.sin_addr, ip_out, 64);
            found = 1;
            break;
        }
    }

    close(sock);
    return found;
}

smart_hub_driver_t hue_driver = {
    .name = "Philips Hue",
    .discover = hue_discover,
    .pair = hue_pair,
    .get_devices = hue_get_devices,
    .set_state = hue_set_state,
    .get_info = hue_get_info
};

bool find_hue_hub(char *ip_out) {
    return hue_discover(ip_out);
}

