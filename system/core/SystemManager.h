#pragma once
#include <Arduino.h>
#include "models/WeatherData.h"
#include "models/DeviceStatus.h"

class SystemManager {
public:
    static void init();
    static void run();              // dipanggil di loop()
    static void receiveWeather();
    static void readLight();
    static void storeData();
    static void upload();
private:
    static WeatherData lastWeather;
    static DeviceStatus status;
    static uint32_t lastUploadTime;
};