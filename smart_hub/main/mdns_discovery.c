#include "mdns_discovery.h"
#include "device_control.h"
#include "esp_log.h"
#include "mdns.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "mdns_discovery";

static void add_mdns_device(const char* hostname, const char* instance, const char* service) {
    device_t dev = {0};
    snprintf(dev.id, sizeof(dev.id), "mdns_%s", hostname);
    snprintf(dev.name, sizeof(dev.name), "%s (%s)", instance, hostname);
    if (strstr(service, "_hue")) {
        strcpy(dev.type, "light");
    } else if (strstr(service, "_googlecast")) {
        strcpy(dev.type, "media");
    } else if (strstr(service, "_hap")) {
        strcpy(dev.type, "apple");
    } else {
        strcpy(dev.type, "unknown");
    }
    strcpy(dev.proto, "mdns");
    strcpy(dev.status, "unavailable");
    snprintf(dev.extra, sizeof(dev.extra), "service=%s", service);
    device_add(&dev);
}

static void mdns_scan_task(void *param) {
    while (1) {
        // Caută serviciile cunoscute: Hue, GoogleCast, Homekit, etc.
        struct {
            const char* service;
            const char* proto;
        } services[] = {
            {"_hue._tcp", "_tcp"},
            {"_googlecast._tcp", "_tcp"},
            {"_hap._tcp", "_tcp"},
        };
        for (int s = 0; s < sizeof(services)/sizeof(services[0]); ++s) {
            mdns_result_t * results = NULL;
            esp_err_t err = mdns_query_ptr(services[s].service, services[s].proto, 3000, 10, &results);
            if (err == ESP_OK && results) {
                mdns_result_t *r = results;
                while (r) {
                    ESP_LOGI(TAG, "mDNS found: %s (%s) [%s]", r->hostname, r->instance_name, services[s].service);
                    add_mdns_device(r->hostname, r->instance_name, services[s].service);
                    r = r->next;
                }
                mdns_query_results_free(results);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // scan la 10 secunde
    }
}

void mdns_discovery_init(void) {
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("esp32-hub"));
    ESP_ERROR_CHECK(mdns_instance_name_set("Smart Hub ESP32"));

    // pornește scanare periodică în task separat
    xTaskCreate(mdns_scan_task, "mdns_scan_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "mDNS discovery started.");
}
