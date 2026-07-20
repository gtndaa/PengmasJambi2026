#include "RTCMemory.h"
#include <esp_sleep.h>

RTC_DATA_ATTR static RTCMemory::RTCData rtcStore;

RTCMemory::RTCData* RTCMemory::rtcData = &rtcStore;

void RTCMemory::init() {
    // Data sudah ada di RTC, tidak perlu inisialisasi ulang
    // Tapi pastikan nilai default saat pertama kali
    if (rtcData->bootCount == 0 && rtcData->wakeCounter == 0) {
        // mungkin first boot
        rtcData->configVersion = 0;
        rtcData->bufferLen = 0;
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