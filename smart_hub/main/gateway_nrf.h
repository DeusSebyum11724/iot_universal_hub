#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Inițializează interfața cu nRF52840 (ex: UART)
void gateway_nrf_init(void);

// Trimite comenzi către nRF52840 (ex: pentru scanare sau control device BLE/Zigbee)
int gateway_nrf_send_cmd(const char* cmd, char* resp_buf, int resp_bufsize);

// Exemplu: adaugă device Zigbee/BLE descoperit (apelată din task-ul de răspuns)
void gateway_nrf_device_add(const char* id, const char* name, const char* type, const char* status);

#ifdef __cplusplus
}
#endif
