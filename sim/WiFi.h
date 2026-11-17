#pragma once
#include "Arduino.h"
#define WIFI_STA 1
struct WiFiC{void mode(int){} bool disconnect(){return true;} String macAddress(){return String{"AA:BB:CC:DD:EE:FF"};}}; extern WiFiC WiFi;
