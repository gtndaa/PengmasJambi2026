#pragma once
#include <Arduino.h>
#include "models/WeatherData.h"

class WeatherDecoder {
public:
    // Mengubah array pulsa menjadi bit (0/1)
    uint16_t pulsesToBits(uint32_t* pulses, uint16_t count, uint8_t* bits, uint16_t maxBits);

    // Mencari paket valid di dalam aliran bit
    bool scanForPacket(uint8_t* bits, uint16_t bitCount, uint8_t* out, uint8_t* outLen);

    // Mendekode paket menjadi WeatherData (termasuk rain delta dan akumulasi)
    bool decodePacket(uint8_t* packet, uint8_t len, WeatherData& data, float& rainAccumulated);

    // Hitung CRC-8 (polynomial 0x31)
    uint8_t crc8(uint8_t* data, uint8_t len);

    // Reset state rain counter (dipanggil saat boot)
    void resetRainCounter(uint8_t initialCounter = RAIN_UNINIT);

private:
    uint8_t rainCounterPrev = RAIN_UNINIT;
    float rainAccumulated = 0.0f;
};