#pragma once
#include <Arduino.h>

// Menyimpan data di RTC memory ESP32 (tidak hilang saat deep sleep)
class RTCMemory {
public:
    static void init();          // panggil di setup
    static uint32_t getBootCount();
    static uint32_t getWakeCounter();
    static void incrementBootCount();
    static void incrementWakeCounter();
    static void setConfigVersion(uint8_t ver);
    static uint8_t getConfigVersion();
    static void saveBuffer(uint8_t* data, size_t len);
    static bool loadBuffer(uint8_t* data, size_t maxLen, size_t& outLen);
private:
    struct RTCData {
        uint32_t bootCount;
        uint32_t wakeCounter;
        uint8_t  configVersion;
        uint8_t  buffer[256];
        uint16_t bufferLen;
    };
    static RTCData* rtcData;
};