#include "headers.h"

uint32_t Scheduler::lastListenTime = 0;
uint32_t Scheduler::lastUploadTime = 0;

void Scheduler::setNextWake(uint32_t intervalMs) {
    // Disimpan lewat RTCMemory (bukan variabel statis biasa) karena
    // deep sleep menghapus RAM. Fungsi ini dipertahankan untuk
    // kompatibilitas API; nilai efektif dihitung di
    // SystemManager::computeNextSleepMs().
    (void)intervalMs;
}

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
