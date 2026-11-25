#pragma once
#include "Arduino.h"
#define WIFI_STA 1
struct StrS { const char *c_str() { return "AA:BB:CC:00:11:22"; } };
struct WiFiClass { void mode(int) {} void disconnect() {} StrS macAddress() { return {}; } }; extern WiFiClass WiFi;
