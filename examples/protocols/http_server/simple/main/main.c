#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "cJSON.h"
#include "esp_http_server.h"

#include "hub_interface.h"
#include "hue.h"
#include "tradfri.h"


#define WIFI_SSID "Asbri's iPhone"
#define WIFI_PASS "EC6E028072"
#define WIFI_CONNECTED_BIT BIT0

typedef enum {
    HUB_TYPE_HUE,
    HUB_TYPE_TRADFRI,
    HUB_TYPE_UNKNOWN
} hub_type_t;

typedef struct {
    char ip[64];
    hub_type_t type;
} hub_info_t;

#define MAX_HUBS 8
hub_info_t discovered_hubs[MAX_HUBS];
int hub_count = 0;


static EventGroupHandle_t wifi_event_group;
static const char *TAG = "smart-home";

char hub_ip[64] = {0};
char hub_user[64] = {0};
smart_hub_driver_t* current_driver = &hue_driver;

extern const char _binary_web_ui_html_start[];
extern const char _binary_web_ui_html_end[];

esp_err_t web_ui_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    size_t html_len = _binary_web_ui_html_end - _binary_web_ui_html_start;
    return httpd_resp_send(req, (const char *)_binary_web_ui_html_start, html_len);
}

static esp_err_t discover_handler(httpd_req_t *req) {
    if (current_driver->discover(hub_ip)) {
        ESP_LOGI(TAG, "Hub găsit la %s", hub_ip);
        return httpd_resp_send(req, hub_ip, HTTPD_RESP_USE_STRLEN);
    } else {
        return httpd_resp_send(req, "Hub not found", HTTPD_RESP_USE_STRLEN);
    }
}

static esp_err_t pair_handler(httpd_req_t *req) {
    if (!strlen(hub_ip)) return httpd_resp_send(req, "IP missing", HTTPD_RESP_USE_STRLEN);
    if (current_driver->pair(hub_ip, hub_user)) {
        ESP_LOGI(TAG, "Pairing OK. User = %s", hub_user);
        return httpd_resp_send(req, hub_user, HTTPD_RESP_USE_STRLEN);
    } else {
        return httpd_resp_send(req, "Pairing failed", HTTPD_RESP_USE_STRLEN);
    }
}

static esp_err_t list_handler(httpd_req_t *req) {
    static char json[4096];
    if (!strlen(hub_ip) || !strlen(hub_user)) return httpd_resp_send(req, "Missing IP/user", HTTPD_RESP_USE_STRLEN);
    if (current_driver->get_devices(hub_ip, hub_user, json, sizeof(json))) {
        return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    } else {
        return httpd_resp_send(req, "Device list failed", HTTPD_RESP_USE_STRLEN);
    }
}

esp_err_t set_handler(httpd_req_t *req) {
    char buf[512];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    buf[len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    const char *id = cJSON_GetObjectItem(root, "id")->valuestring;
    cJSON *state = cJSON_GetObjectItem(root, "state");
    if (!id || !state) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing id/state");
        return ESP_FAIL;
    }

    char *state_str = cJSON_PrintUnformatted(state);
    if (!state_str) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "State error");
        return ESP_FAIL;
    }

    if (!strlen(hub_ip) || !strlen(hub_user)) {
        free(state_str);
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing IP/user");
        return ESP_FAIL;
    }

    if (!hue_set_state(hub_ip, hub_user, id, state_str)) {
        free(state_str);
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to set");
        return ESP_FAIL;
    }

    free(state_str);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static esp_err_t info_handler(httpd_req_t *req) {
    static char json[2048];
    if (!strlen(hub_ip) || !strlen(hub_user)) return httpd_resp_send(req, "{}", HTTPD_RESP_USE_STRLEN);
    if (current_driver->get_info(hub_ip, hub_user, json, sizeof(json))) {
        return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    } else {
        return httpd_resp_send(req, "{}", HTTPD_RESP_USE_STRLEN);
    }
}

extern bool find_hue_hub(char *out_ip);
extern bool find_tradfri_gateway(char *out_ip);

static esp_err_t discover_all_handler(httpd_req_t *req) {
    hub_count = 0;

    if (find_hue_hub(discovered_hubs[hub_count].ip)) {
        discovered_hubs[hub_count].type = HUB_TYPE_HUE;
        hub_count++;
    }

    if (find_tradfri_gateway(discovered_hubs[hub_count].ip)) {
        discovered_hubs[hub_count].type = HUB_TYPE_TRADFRI;
        hub_count++;
    }

    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < hub_count; i++) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "ip", discovered_hubs[i].ip);
        cJSON_AddStringToObject(obj, "type", 
            discovered_hubs[i].type == HUB_TYPE_HUE ? "hue" :
            discovered_hubs[i].type == HUB_TYPE_TRADFRI ? "tradfri" : "unknown");
        cJSON_AddItemToArray(arr, obj);
    }

    char *json = cJSON_PrintUnformatted(arr);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    free(json);
    cJSON_Delete(arr);
    return ESP_OK;
}


static void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/", .method = HTTP_GET, .handler = web_ui_get_handler });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/discover", .method = HTTP_POST, .handler = discover_handler });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/pair", .method = HTTP_POST, .handler = pair_handler });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/list", .method = HTTP_POST, .handler = list_handler });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/set", .method = HTTP_POST, .handler = set_handler });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/info", .method = HTTP_GET, .handler = info_handler });
        httpd_register_uri_handler(server, &(httpd_uri_t){
    .uri = "/discover_all",
    .method = HTTP_POST,
    .handler = discover_all_handler,
    .user_ctx = NULL
});


        ESP_LOGI(TAG, "Webserver started cu toate rutele înregistrate!");
    } else {
        ESP_LOGE(TAG, "Failed to start webserver");
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) esp_wifi_connect();
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
}

static void wifi_init(void) {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    start_webserver();
    ESP_LOGI(TAG, "ESP32 Smart Hub ready!");
}
