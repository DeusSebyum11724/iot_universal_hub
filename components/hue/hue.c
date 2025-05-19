// hue.c
#include "hue.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include \"hub_interface.h\"

smart_hub_driver_t hue_driver = {
    .name = \"Philips Hue\",
    .discover = hue_discover,
    .pair = hue_pair,
    .get_devices = hue_get_devices,
    .set_state = hue_set_state,
    .get_info = hue_get_info
};

extern int http_raw_request(const char* ip, int port, const char* request, char* response, size_t max_resp_len);

bool hue_pair(const char* ip, char* user_out) {
    const char* post_data = "{\"devicetype\":\"esp32_hub#nasu\"}";
    char http_req[512], response[2048];
    snprintf(http_req, sizeof(http_req),
        "POST /api HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n"
        "%s", ip, (int)strlen(post_data), post_data);

    int len = http_raw_request(ip, 80, http_req, response, sizeof(response));
    if (len <= 0) return false;

    char* json_start = strchr(response, '[');
    if (!json_start) return false;

    char* usr = strstr(json_start, "\"username\":\"");
    if (usr) {
        usr += strlen("\"username\":\"");
        char* end = strchr(usr, '"');
        if (end && end - usr < 64) {
            strncpy(user_out, usr, end - usr);
            user_out[end - usr] = '\0';
            return true;
        }
    }
    return false;
}

bool hue_get_devices(const char* ip, const char* user, char* json_out, size_t max_len) {
    char http_req[256], response[4096];
    snprintf(http_req, sizeof(http_req),
        "GET /api/%s/lights HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n", user, ip);

    int len = http_raw_request(ip, 80, http_req, response, sizeof(response));
    if (len <= 0) return false;

    char* json_start = strchr(response, '{');
    if (!json_start) return false;

    strncpy(json_out, json_start, max_len - 1);
    json_out[max_len - 1] = '\0';
    return true;
}

bool hue_set_state(const char* ip, const char* user, const char* device_id, const char* state_json) {
    char http_req[1024], response[2048];
    snprintf(http_req, sizeof(http_req),
        "PUT /api/%s/lights/%s/state HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        user, device_id, ip, (int)strlen(state_json), state_json);

    int len = http_raw_request(ip, 80, http_req, response, sizeof(response));
    return (len > 0);
}

bool hue_get_info(const char* ip, const char* user, char* json_out, size_t max_len) {
    char http_req[256], response[4096];
    snprintf(http_req, sizeof(http_req),
        "GET /api/%s/config HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n", user, ip);

    int len = http_raw_request(ip, 80, http_req, response, sizeof(response));
    if (len <= 0) return false;

    char* json_start = strchr(response, '{');
    if (!json_start) return false;

    strncpy(json_out, json_start, max_len - 1);
    json_out[max_len - 1] = '\0';
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

    struct sockaddr_in dest_addr = { .sin_family = AF_INET, .sin_port = htons(1900) };
    inet_pton(AF_INET, "239.255.255.250", &dest_addr.sin_addr);

    struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    sendto(sock, msearch, strlen(msearch), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

    char recv_buf[1024];
    struct sockaddr_in source_addr;
    socklen_t addr_len = sizeof(source_addr);
    int found = 0;

    while (recvfrom(sock, recv_buf, sizeof(recv_buf) - 1, 0, (struct sockaddr*)&source_addr, &addr_len) > 0) {
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