#include "Scheduler.h"
#include "config.h"
#include "power/PowerManager.h"
#include "utils/Logger.h"

uint32_t Scheduler::lastListenTime = 0;
uint32_t Scheduler::lastUploadTime = 0;

void Scheduler::setNextWake(uint32_t intervalMs) {
    // disimpan untuk digunakan di goToSleep()
    // kita akan gunakan PowerManager
}

void Scheduler::goToSleep() {
    PowerManager pm;
    pm.prepareDeepSleep(SLEEP_INTERVAL_MS * 1000ULL);
    LOG_INFO("Entering deep sleep for %d ms", SLEEP_INTERVAL_MS);
    pm.deepSleepNow();
}

bool Scheduler::isTimeToUpload() {
    return (millis() - lastUploadTime >= UPLOAD_INTERVAL_MS);
}

bool Scheduler::isTimeToListen() {
    return (millis() - lastListenTime >= LISTEN_WINDOW_MS);
}

void Scheduler::resetListenTimer() {
    lastListenTime = millis();
}

void Scheduler::resetUploadTimer() {
    lastUploadTime = millis();
}