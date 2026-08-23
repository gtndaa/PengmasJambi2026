#include "headers.h"

void Scheduler::goToSleep() {
    // Durasi dihitung berdasarkan hasil siklus terakhir (sinkronisasi
    // jadwal paket 48 detik, lihat SystemManager::computeNextSleepMs()).
    uint32_t sleepMs = SystemManager::computeNextSleepMs();
    goToSleep(sleepMs);
}

void Scheduler::goToSleep(uint32_t sleepMs) {
    if (sleepMs < MIN_SLEEP_MS) sleepMs = MIN_SLEEP_MS;

    // Matikan periferal yang boros arus sebelum tidur.
    CC1101Driver radio;
    radio.sleepRadio();

    LightSensor light;
    light.powerDown();

    if (WiFi.getMode() != WIFI_OFF) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }

    PowerManager pm;
    pm.prepareDeepSleep((uint64_t)sleepMs * 1000ULL);

    LOG_INFO("Deep sleep %lu ms (sinkron=%s, missed=%d, wake#%lu)",
             (unsigned long)sleepMs,
             RTCMemory::isSynced() ? "ya" : "tidak",
             RTCMemory::getMissedCycles(),
             (unsigned long)RTCMemory::getWakeCounter());

    Serial.flush();
    pm.deepSleepNow();
}
