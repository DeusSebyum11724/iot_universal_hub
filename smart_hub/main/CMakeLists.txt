set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

idf_component_register(
  SRCS "main.c" "wifi_manager.c" "web_server.c" "ssdp_discovery.c" "gateway_nrf.c" "mdns_discovery.c"
  INCLUDE_DIRS "."
  PRIV_REQUIRES esp_wifi esp_netif esp_event mdns esp_http_server nvs_flash driver

)
