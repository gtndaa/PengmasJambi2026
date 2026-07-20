#include "WeatherDecoder.h"
#include "constants.h"
#include "config.h"

uint16_t WeatherDecoder::pulsesToBits(uint32_t* pulses, uint16_t count,
                                      uint8_t* bits, uint16_t maxBits) {
    uint16_t n = 0;
    for (uint16_t i = 0; i < count && n < maxBits; i++) {
        if      (pulses[i] >= PULSE_1_MIN && pulses[i] <= PULSE_1_MAX) bits[n++] = 1;
        else if (pulses[i] >= PULSE_0_MIN && pulses[i] <= PULSE_0_MAX) bits[n++] = 0;
    }
    return n;
}

uint8_t WeatherDecoder::crc8(uint8_t* data, uint8_t len) {
    uint8_t crc = CRC_INIT;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ CRC_POLY : (crc << 1);
    }
    return crc;
}

bool WeatherDecoder::scanForPacket(uint8_t* bits, uint16_t bitCount,
                                   uint8_t* out, uint8_t* outLen) {
    for (uint16_t offset = 0; offset + 64 <= bitCount; offset++) {
        for (uint8_t nbytes = 8; nbytes <= MAX_BYTES; nbytes++) {
            if (offset + (uint16_t)nbytes * 8 > bitCount) break;
            uint8_t candidate[MAX_BYTES] = {0};
            for (uint8_t b = 0; b < nbytes; b++) {
                uint8_t byte = 0;
                for (uint8_t bit = 0; bit < 8; bit++)
                    byte = (byte << 1) | bits[offset + b * 8 + bit];
                candidate[b] = byte;
            }
            if (crc8(candidate, nbytes - 1) == candidate[nbytes - 1] &&
                candidate[0] == 0xAA) {
                memcpy(out, candidate, nbytes);
                *outLen = nbytes;
                return true;
            }
        }
    }
    return false;
}

void WeatherDecoder::resetRainCounter(uint8_t initialCounter) {
    rainCounterPrev = initialCounter;
    rainAccumulated = 0.0f;
}

bool WeatherDecoder::decodePacket(uint8_t* packet, uint8_t len, WeatherData& data, float& rainAccumulatedRef) {
    if (len < 10) return false;

    data.sensorId   = packet[1];
    uint16_t rawT   = ((uint16_t)(packet[1] & 0x0F) << 8) | packet[2];
    data.temperature = (rawT - TEMP_OFFSET) / TEMP_DIVISOR;
    data.humidity   = packet[3];
    data.windSpeed  = packet[4] * WIND_FACTOR;
    data.windGust   = packet[5] * WIND_FACTOR;
    uint8_t rainRaw = packet[6];
    data.batteryOk  = (packet[7] & 0x80) != 0;
    data.channel    = (packet[7] >> 4) & 0x07;
    data.windDirection = packet[8] & 0x0F;
    data.windDeg    = data.windDirection * WIND_DEG_STEP;
    data.rainRaw    = rainRaw;
    data.light      = 0; // akan diisi dari sensor terpisah

    // Hitung delta rain dengan wrap-around
    float delta = 0.0f;
    if (rainCounterPrev != RAIN_UNINIT) {
        uint8_t prev = rainCounterPrev;
        if (rainRaw != prev) {
            uint8_t diff = (rainRaw > prev) ? (rainRaw - prev) : (uint8_t)(256 - prev + rainRaw);
            delta = diff * RAIN_MM_PER_TIP;
            rainAccumulated += delta;
        }
    }
    rainCounterPrev = rainRaw;
    data.rainDelta = delta;
    data.rainTotal = rainAccumulated;
    rainAccumulatedRef = rainAccumulated;
    return true;
}