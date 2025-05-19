// hub_interface.h
#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char* name;
    bool (*discover)(char* ip_out);
    bool (*pair)(const char* ip, char* user_out);
    bool (*get_devices)(const char* ip, const char* user, char* json_out, size_t max_len);
    bool (*set_state)(const char* ip, const char* user, const char* device_id, const char* state_json);
    bool (*get_info)(const char* ip, const char* user, char* json_out, size_t max_len);
} smart_hub_driver_t;
