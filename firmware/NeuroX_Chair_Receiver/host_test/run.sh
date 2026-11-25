#!/bin/sh
# Runs the chair receiver on the PC with a fake ESP32 + radio and checks 22 safety behaviours.
cd "$(dirname "$0")" && g++ -std=gnu++17 -Wall -Wno-unused-function -Wno-unused-variable -x c++ -I stub test_chair.cpp -o /tmp/test_chair && /tmp/test_chair
