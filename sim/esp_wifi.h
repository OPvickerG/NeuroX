#pragma once
#include <stdint.h>
#define WIFI_PS_NONE 0
#define WIFI_SECOND_CHAN_NONE 0
inline int esp_wifi_set_ps(int){return 0;} inline int esp_wifi_set_channel(int,int){return 0;}
static int8_t g_txp = 80;
inline int esp_wifi_set_max_tx_power(int8_t p){ g_txp = p; return 0; }
inline int esp_wifi_get_max_tx_power(int8_t* p){ *p = g_txp; return 0; }
