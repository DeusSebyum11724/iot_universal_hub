// http_utils.h
#pragma once

#include <stddef.h>

// Trimite un request HTTP direct prin socket și primește răspunsul
int http_raw_request(const char* ip, int port, const char* request, char* response, size_t max_resp_len);
