#pragma once
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK 0
#define WIFI_IF_STA 0
typedef struct {uint8_t peer_addr[6]; uint8_t channel; int ifidx; bool encrypt;} esp_now_peer_info_t;
inline esp_err_t esp_now_init(){return ESP_OK;} inline esp_err_t esp_now_add_peer(const esp_now_peer_info_t*){return ESP_OK;}
esp_err_t esp_now_send(const uint8_t*,const uint8_t*,size_t);
