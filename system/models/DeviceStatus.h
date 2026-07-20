#pragma once
#include <Arduino.h>

struct DeviceStatus {
    float  batteryVoltage;       // Volt
    float  superCapVoltage;      // Volt
    int8_t wifiRSSI;             // dBm
    uint32_t freeHeap;
    uint32_t uptime;             // detik
    uint32_t bootCount;
    uint32_t wakeCounter;
    char   firmwareVersion[16];
    bool   sdCardPresent;
    bool   rtcPresent;
    bool   lightSensorPresent;
    bool   radioPresent;
};