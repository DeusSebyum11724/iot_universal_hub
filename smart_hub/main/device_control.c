#include "device_control.h"
#include <string.h>
#include <stdio.h>

device_t devices[MAX_DEVICES];
int device_count = 0;

void device_control_init(void) {
    device_count = 0;
    memset(devices, 0, sizeof(devices));
}

// Adaugă un device dacă nu există deja (după id)
int device_add(const device_t *dev) {
    for (int i = 0; i < device_count; ++i) {
        if (strcmp(devices[i].id, dev->id) == 0) {
            // deja există, updatează info
            devices[i] = *dev;
            return i;
        }
    }
    if (device_count < MAX_DEVICES) {
        devices[device_count++] = *dev;
        return device_count - 1;
    }
    // lista plină
    return -1;
}

// Găsește pointer la device după id
device_t* device_find_by_id(const char* id) {
    for (int i = 0; i < device_count; ++i) {
        if (strcmp(devices[i].id, id) == 0) {
            return &devices[i];
        }
    }
    return NULL;
}

// Exemplu simplu: toggle (ON <-> OFF)
int device_toggle(const char* id) {
    device_t* dev = device_find_by_id(id);
    if (!dev) return -1;
    if (strcmp(dev->status, "on") == 0) {
        strcpy(dev->status, "off");
    } else {
        strcpy(dev->status, "on");
    }
    // TODO: Trimitere comandă reală către device (ex: HTTP/REST la Hue, etc)
    return 0;
}
