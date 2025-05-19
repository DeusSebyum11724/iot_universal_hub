#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <string.h>

// Pentru a servi index.html ca resursă binară
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;

// --- HANDLER UI ---
static esp_err_t index_get_handler(httpd_req_t *req) {
    const uint8_t *html = index_html_start;
    size_t html_len = index_html_end - index_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)html, html_len);
    return ESP_OK;
}

httpd_uri_t index_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_get_handler,
    .user_ctx = NULL
};

// --- HANDLER API: /api/devices (dummy pentru test) ---
static esp_err_t api_devices_get_handler(httpd_req_t *req) {
    // Exemplu JSON static, updatează să ia din device_control.c!
    const char* resp = "[{\"id\":\"abc1\",\"name\":\"Hue Bulb\",\"type\":\"light\",\"proto\":\"mdns\",\"status\":\"on\"}]";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

httpd_uri_t api_devices_uri = {
    .uri = "/api/devices",
    .method = HTTP_GET,
    .handler = api_devices_get_handler,
    .user_ctx = NULL
};

void web_server_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &api_devices_uri);
        ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }
}
