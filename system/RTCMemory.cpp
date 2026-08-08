#include "headers.h"
#include <esp_sleep.h>
#include <string.h>

RTC_DATA_ATTR static RTCMemory::RTCData rtcStore;

RTCMemory::RTCData* RTCMemory::rtcData = &rtcStore;

void RTCMemory::init() {
    // Data sudah ada di RTC (bertahan lintas deep-sleep), tidak perlu
    // inisialisasi ulang selama itu bukan power-on pertama kali.
    // Deteksi first-boot: wakeup cause == UNDEFINED artinya power-on/reset,
    // bukan bangun dari deep sleep timer -> reset semua state jadwal.
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
        rtcData->bootCount = 0;
        rtcData->wakeCounter = 0;
        rtcData->configVersion = 0;
        rtcData->bufferLen = 0;
        rtcData->lastPacketEpoch = 0;
        rtcData->missedCycles = 0;
        rtcData->pendingLen = 0;
        rtcData->rainCounterPrev = RAIN_UNINIT;
        rtcData->rainAccumulated = 0.0f;
    }
}

uint32_t RTCMemory::getBootCount() {
    return rtcData->bootCount;
}

uint32_t RTCMemory::getWakeCounter() {
    return rtcData->wakeCounter;
}

void RTCMemory::incrementBootCount() {
    rtcData->bootCount++;
}

void RTCMemory::incrementWakeCounter() {
    rtcData->wakeCounter++;
}

void RTCMemory::setConfigVersion(uint8_t ver) {
    rtcData->configVersion = ver;
}

uint8_t RTCMemory::getConfigVersion() {
    return rtcData->configVersion;
}

void RTCMemory::saveBuffer(uint8_t* data, size_t len) {
    if (len > sizeof(rtcData->buffer)) len = sizeof(rtcData->buffer);
    memcpy(rtcData->buffer, data, len);
    rtcData->bufferLen = len;
}

bool RTCMemory::loadBuffer(uint8_t* data, size_t maxLen, size_t& outLen) {
    outLen = rtcData->bufferLen;
    if (outLen == 0 || outLen > maxLen) return false;
    memcpy(data, rtcData->buffer, outLen);
    return true;
}

// ---------------- sinkronisasi jadwal paket radio ----------------

uint32_t RTCMemory::getLastPacketEpoch() {
    return rtcData->lastPacketEpoch;
}

void RTCMemory::setLastPacketEpoch(uint32_t epoch) {
    rtcData->lastPacketEpoch = epoch;
}

bool RTCMemory::isSynced() {
    return rtcData->lastPacketEpoch != 0 && rtcData->missedCycles < MAX_MISSED_CYCLES;
}

uint8_t RTCMemory::getMissedCycles() {
    return rtcData->missedCycles;
}

void RTCMemory::incrementMissedCycles() {
    if (rtcData->missedCycles < 255) rtcData->missedCycles++;
}

void RTCMemory::resetMissedCycles() {
    rtcData->missedCycles = 0;
}

// ---------------- buffer pending upload ----------------

bool RTCMemory::pushPending(const WeatherData& data) {
    if (rtcData->pendingLen >= PENDING_BUFFER_SIZE) return false;
    rtcData->pending[rtcData->pendingLen++] = data;
    return true;
}

uint8_t RTCMemory::pendingCount() {
    return rtcData->pendingLen;
}

bool RTCMemory::getPending(uint8_t index, WeatherData& out) {
    if (index >= rtcData->pendingLen) return false;
    out = rtcData->pending[index];
    return true;
}

void RTCMemory::clearPending() {
    rtcData->pendingLen = 0;
}

void RTCMemory::compactPending(const bool keep[], uint8_t count) {
    // Menyusun ulang buffer supaya hanya menyisakan entri yang gagal
    // dipindah/dikirim (keep[i]==true), digeser rapat ke depan. Dipakai
    // supaya data yang gagal disimpan ke SD (mis. SPI/SD lagi bentrok
    // dengan radio) tidak ikut hilang namun tetap bertahan di RTC memory
    // untuk dicoba lagi siklus berikutnya
    if (count > rtcData->pendingLen) count = rtcData->pendingLen;
    uint8_t w = 0;
    for (uint8_t i = 0; i < count; i++) {
        if (keep[i]) {
            if (w != i) rtcData->pending[w] = rtcData->pending[i];
            w++;
        }
    }
    // Entri di luar `count` (kalau ada, seharusnya tidak pernah terjadi
    // karena count == pendingLen saat dipanggil) ikut disalin apa adanya.
    for (uint8_t i = count; i < rtcData->pendingLen; i++) {
        rtcData->pending[w++] = rtcData->pending[i];
    }
    rtcData->pendingLen = w;
}

// ---------------- state decoder curah hujan ----------------

uint8_t RTCMemory::getRainCounterPrev() {
    return rtcData->rainCounterPrev;
}

void RTCMemory::setRainCounterPrev(uint8_t v) {
    rtcData->rainCounterPrev = v;
}

float RTCMemory::getRainAccumulated() {
    return rtcData->rainAccumulated;
}

void RTCMemory::setRainAccumulated(float v) {
    rtcData->rainAccumulated = v;
}
