idf_component_register(
    SRCS main.c hue.c http_utils.c tradfri.c 
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_http_server esp_event nvs_flash esp_wifi esp_netif cjson mdns
    EMBED_FILES "web_ui.html"
)
