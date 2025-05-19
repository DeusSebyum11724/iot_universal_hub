#pragma once

#include <stdbool.h>
#include <stddef.h>

// Funcție care descoperă bridge-ul Hue folosind SSDP.
// Returnează true dacă a găsit, iar IP-ul este pus în ip_out (max 64 bytes).
bool hue_discover(char* ip_out);

// Funcție care face pairing cu bridge-ul Hue și obține un username.
// Returnează true dacă pairing-ul a reușit, username-ul este pus în user_out.
bool hue_pair(const char* ip, char* user_out);

// Returnează lista de dispozitive Hue sub formă de JSON, în json_out.
// Returnează true dacă cererea a avut succes.
bool hue_get_devices(const char* ip, const char* user, char* json_out, size_t max_len);

// Trimite o comandă de schimbare stare (on/off, bri etc.) pentru un device dat.
// Returnează true dacă succes.
bool hue_set_state(const char* ip, const char* user, const char* device_id, const char* state_json);

// Returnează informații despre bridge (model, MAC, versiune) sub formă JSON.
bool hue_get_info(const char* ip, const char* user, char* json_out, size_t max_len);

// Driverul Hue complet, expus pentru a fi folosit într-un array de drivere sau ca pointer activ.
#include "hub_interface.h"
extern smart_hub_driver_t hue_driver;
