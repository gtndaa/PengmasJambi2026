#pragma once
#include <Arduino.h>

struct DeviceConfig {
    String wifiSSID;
    String wifiPassword;
    String serverURL;           // untuk HTTP getConfig
    String apiKey;
    String mqttBroker;
    uint16_t mqttPort = 1883;
    String mqttClientId;
    String mqttUsername;
    String mqttPassword;
    unsigned long uploadInterval = 60000;   // ms
    unsigned long listenWindow = 30000;     // ms
};