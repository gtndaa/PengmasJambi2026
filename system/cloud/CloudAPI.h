#pragma once
#include <Arduino.h>
#include "models/WeatherData.h"
#include "models/DeviceStatus.h"

class CloudAPI {
public:
    bool begin(const char* serverURL, const char* apiKey);
    bool uploadWeather(const WeatherData& data);
    bool uploadStatus(const DeviceStatus& status);
    bool getConfig(String& configJSON);   // download konfigurasi dari server
private:
    String server;
    String key;
};