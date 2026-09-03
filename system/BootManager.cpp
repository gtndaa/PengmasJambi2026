#include "headers.h"

void BootManager::init() {
    RTCMemory::init();
    RTCMemory::incrementBootCount();
    LOG_INFO("Boot #%d", RTCMemory::getBootCount());
    LOG_INFO("Firmware: %s v%s", PROJECT_NAME, FW_VERSION);
    LOG_INFO("Free heap: %d", ESP.getFreeHeap());

    // Deteksi penyebab reset SPESIFIK (beda dengan cek cold-boot vs
    // deep-sleep-wake yang sudah ada di RTCMemory::init()). esp_reset_reason()
    // bisa membedakan brownout (tegangan sempat drop di bawah ambang
    // detektor internal ESP32) dari power-on normal & penyebab lain.

    esp_reset_reason_t resetReason = esp_reset_reason();
    LOG_INFO("Reset reason=%d (0=UNKNOWN 1=POWERON 3=SW 4=PANIC 5=INT_WDT "
              "6=TASK_WDT 7=WDT 8=DEEPSLEEP 9=BROWNOUT 10=SDIO)", (int)resetReason);

    if (resetReason == ESP_RST_BROWNOUT) {
        LOG_WARN("Reset reason=BROWNOUT. Sistem butuh NTP resync");
        RTCManager::markNeedResync();
    }
}

uint32_t BootManager::getBootCount() {
    return RTCMemory::getBootCount();
}