#pragma once
#include <cstdint>
#define ESP_OK 0
struct esp_now_recv_info_t { uint8_t *src_addr; };
typedef void (*esp_now_recv_cb_t)(const esp_now_recv_info_t *, const uint8_t *, int);
extern esp_now_recv_cb_t g_recv;
inline int esp_now_init() { return ESP_OK; }
inline int esp_now_register_recv_cb(esp_now_recv_cb_t cb) { g_recv = cb; return ESP_OK; }
