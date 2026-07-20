#include "BootManager.h"
#include "storage/RTCMemory.h"
#include "utils/Logger.h"
#include "config.h"

void BootManager::init() {
    RTCMemory::init();
    RTCMemory::incrementBootCount();
    LOG_INFO("Boot #%d", RTCMemory::getBootCount());
    LOG_INFO("Firmware: %s v%s", PROJECT_NAME, FW_VERSION);
    LOG_INFO("Free heap: %d", ESP.getFreeHeap());
}

void BootManager::printSystemInfo() {
    Serial.printf("Project: %s v%s\n", PROJECT_NAME, FW_VERSION);
    Serial.printf("Boot count: %d\n", RTCMemory::getBootCount());
    Serial.printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Heap: %d\n", ESP.getFreeHeap());
}

bool BootManager::isFirstBoot() {
    return RTCMemory::getBootCount() == 1;
}

uint32_t BootManager::getBootCount() {
    return RTCMemory::getBootCount();
}