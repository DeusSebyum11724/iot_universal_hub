// http_utils.c
#include "http_utils.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

int http_raw_request(const char* ip, int port, const char* request, char* response, size_t max_resp_len) {
    struct sockaddr_in server_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        close(sock);
        return -2;
    }

    int sent = send(sock, request, strlen(request), 0);
    if (sent < 0) {
        close(sock);
        return -3;
    }

    int total = 0, r;
    while ((r = recv(sock, response + total, max_resp_len - total - 1, 0)) > 0) {
        total += r;
        if (total >= max_resp_len - 1) break;
    }
    response[total] = 0;
    close(sock);
    return total;
}
