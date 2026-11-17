#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include <string>
#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 0x05
int digitalRead(uint8_t pin);
#define OUTPUT 1
#define ADC_11db 3
uint32_t millis(); uint32_t micros(); void delay(uint32_t);
int analogRead(uint8_t); inline void analogReadResolution(uint8_t){} inline void analogSetPinAttenuation(uint8_t,int){}
inline void pinMode(uint8_t,uint8_t){} void digitalWrite(uint8_t,uint8_t);
struct String{std::string s; const char* c_str() const {return s.c_str();}};
struct HS{void begin(unsigned long){} int available(); int read(); int availableForWrite(){return 256;} size_t write(const uint8_t*,size_t);};
extern HS Serial;

double ledcSetup(uint8_t ch, double f, uint8_t bits); void ledcAttachPin(uint8_t pin, uint8_t ch); void ledcWrite(uint8_t ch, uint32_t duty);
