#include "headers.h"

void BootManager::init() {
    PersistentState::init();
    LOG_INFO("Boot #%lu", (unsigned long)PersistentState::getBootCount());
    LOG_INFO("Firmware: %s v%s", PROJECT_NAME, FW_VERSION);
    LOG_INFO("Free heap: %d", ESP.getFreeHeap());

    // Setiap boot di mode kontinu itu selalu berarti power-on/reset
    // sungguhan (tidak ada lagi "wake dari deep sleep" untuk dibedakan),
    // jadi selalu tandai butuh resync waktu dari NTP -- akan
    // dikonsumsi & dieksekusi SystemManager begitu WiFi pertama kali
    // konek.
    RTCManager::markNeedResync();
}

uint32_t BootManager::getBootCount() {
    return PersistentState::getBootCount();
}
