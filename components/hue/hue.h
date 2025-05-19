#pragma once

#include <stdbool.h>
#include <stddef.h>

bool hue_discover(char* ip_out);
bool hue_pair(const char* ip, char* user_out);
bool hue_get_devices(const char* ip, const char* user, char* json_out, size_t max_len);
bool hue_set_state(const char* ip, const char* user, const char* device_id, const char* state_json);
bool hue_get_info(const char* ip, const char* user, char* json_out, size_t max_len);
