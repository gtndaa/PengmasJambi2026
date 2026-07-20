#pragma once
#include <Arduino.h>
#include <RTClib.h>

class RTCManager {
public:
    bool begin();
    bool isOK() const;
    DateTime now();
    float getTemperature();
    String dateTimeStr();
    String isoStr();
    void adjust(const DateTime& dt);
private:
    RTC_DS3231 rtc;
    bool ok = false;
};