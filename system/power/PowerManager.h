#pragma once
#include <Arduino.h>

class PowerManager {
public:
    void begin();
    float readBatteryVoltage();      // volt
    float readSuperCapVoltage();     // volt
    void prepareDeepSleep(uint64_t wakeUpTimeUs);
    void deepSleepNow();
    bool isLowPower() const;
    void setSleepInterval(uint64_t us);
private:
    uint64_t sleepInterval = 0;
    uint32_t lastWake = 0;
};