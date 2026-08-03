#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>

struct DeviceConfig
{
    String wifiSSID;
    String wifiPassword;

    uint32_t uploadInterval;
    uint32_t listenWindow;

    int timezoneOffset;

    uint8_t configVersion;

    bool useDeepSleep;
};

#endif