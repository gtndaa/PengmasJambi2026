#pragma once
#include <Arduino.h>
#include <IPAddress.h>

struct DeviceConfig {
    char wifiSSID[32];
    char wifiPassword[64];
    char serverURL[128];
    char apiKey[64];
    uint32_t uploadInterval;      // ms
    uint32_t listenWindow;        // ms
    int32_t  timezoneOffset;      // detik
    uint8_t  configVersion;
    bool     useDeepSleep;
};