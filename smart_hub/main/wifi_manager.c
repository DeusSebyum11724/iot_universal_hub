// wifi_manager.c - Configurare și inițializare Wi-Fi STA

#include <string.h>
#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"

#define WIFI_SSID "Amos_2.4G"    // <- inlocuiți cu SSID-ul real
#define WIFI_PASS "EC6E028072"    // <- inlocuiți cu parola reală

static const char *TAG = "wifi_manager";
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;  // semnalizează conexiune stabilită

// Event handler pentru evenimente Wi-Fi
static void wifi_event_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();  // începe conectarea la AP
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Deconectat de la WiFi, incerc reconectarea...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectat, adresa IP obtinuta: %s", ip4addr_ntoa(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init() {
    // Inițializează stocarea NVS (necesar de obicei pentru WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Șterge NVS dacă e corupt
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Inițializează stack-ul de rețea (TCP/IP)
    ESP_ERROR_CHECK(esp_netif_init());
    // Inițializează loop-ul de evenimente
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Creează interfața de rețea STA (stație WiFi)
    esp_netif_create_default_wifi_sta();

    // Configurare Wi-Fi 
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // Înregistrează event handler pentru Wi-Fi și IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_event_handler,
                    NULL,
                    NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler,
                    NULL,
                    NULL));

    // Setează modul STA și SSID/parola
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK // poate fi ajustat după caz
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Creează un event group pentru a sincroniza conectarea
    wifi_event_group = xEventGroupCreate();

    // Pornește Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi init complet, se conecteaza la %s...", WIFI_SSID);
}
