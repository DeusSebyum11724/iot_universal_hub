#include "wifi_manager.h"
#include "web_server.h"
#include "mdns_discovery.h"
#include "ssdp_discovery.h"
#include "gateway_nrf.h"
#include "device_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void app_main(void) {
    // 1. Inițializează control device (structuri globale, etc.)
    device_control_init();

    // 2. Conectează la WiFi (schimbă SSID/Password în wifi_manager.c)
    wifi_manager_init();

    // 3. Pornește serverul web + REST API + UI
    web_server_start();

    // 4. Pornește mDNS discovery (Philips Hue, GoogleCast, etc)
    mdns_discovery_init();

    // 5. Pornește SSDP discovery (TV, PlayStation, AV, etc)
    ssdp_discovery_init();

    // 6. Pornește gateway nRF52840 (pentru BLE/Zigbee devices)
    gateway_nrf_init();

    // 7. Loop principal (nu e nevoie de cod aici, task-urile rulează independent)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000)); // doar să nu iasă din funcție
    }
}
