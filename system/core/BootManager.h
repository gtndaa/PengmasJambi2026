#pragma once
#include <Arduino.h>

class BootManager {
public:
    static void init();
    static void printSystemInfo();
    static bool isFirstBoot();
    static uint32_t getBootCount();
};