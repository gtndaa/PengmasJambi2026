#pragma once
#include <Arduino.h>

class Scheduler {
public:
    static void setNextWake(uint32_t intervalMs);
    static void goToSleep();
    static bool isTimeToUpload();
    static bool isTimeToListen();
    static void resetListenTimer();
    static void resetUploadTimer();
private:
    static uint32_t lastListenTime;
    static uint32_t lastUploadTime;
};