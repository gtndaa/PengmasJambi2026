#include "headers.h"

void BootManager::init() {
    RTCMemory::init();
    RTCMemory::incrementBootCount();
    LOG_INFO("Boot #%d", RTCMemory::getBootCount());
    LOG_INFO("Firmware: %s v%s", PROJECT_NAME, FW_VERSION);
    LOG_INFO("Free heap: %d", ESP.getFreeHeap());
}

uint32_t BootManager::getBootCount() {
    return RTCMemory::getBootCount();
}