#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdarg>
#include <string>
#include <deque>
#include <algorithm>
#include <initializer_list>
using std::min; using std::max;
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define ESP_ARDUINO_VERSION_MAJOR 3
extern uint32_t g_ms; extern int g_dig[64]; extern uint32_t g_duty[64]; extern bool g_attached[64];
inline uint32_t millis() { return g_ms; }
inline void delay(uint32_t ms) { g_ms += ms; }
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t p, uint8_t v) { g_dig[p] = v; }
inline bool ledcAttach(uint8_t p, uint32_t, uint8_t) { g_attached[p] = true; return true; }
inline bool ledcWrite(uint8_t p, uint32_t d) { g_duty[p] = d; return true; }
template <class T> T constrain(T x, T a, T b) { return x < a ? a : x > b ? b : x; }
struct HWSerial { std::deque<char> in; bool quiet = false;
  void begin(int) {} int available() { return (int)in.size(); } int read() { char c = in.front(); in.pop_front(); return c; }
  void println(const char *s = "") { if (!quiet) printf("%s\n", s); }
  void printf(const char *f, ...) { if (quiet) return; va_list a; va_start(a, f); vprintf(f, a); va_end(a); } };
extern HWSerial Serial;
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x)
#define portEXIT_CRITICAL(x)
struct EspClass { void restart() {} }; extern EspClass ESP;
