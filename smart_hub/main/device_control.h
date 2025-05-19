#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DEVICES 32

typedef struct {
    char id[32];
    char name[64];
    char type[16];    // "light", "plug", "tv", "sensor", etc.
    char proto[16];   // "mdns", "ssdp", "ble", "zigbee", etc.
    char status[16];  // "on", "off", "unavailable", etc.
    char extra[128];  // orice info suplimentar (IP, MAC, etc)
} device_t;

// Lista globală cu device-urile descoperite
extern device_t devices[MAX_DEVICES];
extern int device_count;

// Inițializează modulul și lista de device-uri
void device_control_init(void);

// Adaugă un device nou (dacă nu există deja cu același id)
int device_add(const device_t *dev);

// Găsește device după id
device_t* device_find_by_id(const char* id);

// Schimbă status-ul (ex: ON/OFF) al unui device, folosit de web_server pentru toggle
int device_toggle(const char* id);

#ifdef __cplusplus
}
#endif
