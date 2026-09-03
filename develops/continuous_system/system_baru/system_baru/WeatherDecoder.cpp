#include "headers.h"

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

void WeatherDecoder::addToTieredAccumulators(float deltaMm, uint32_t epoch) {
    // ---- rolling 1 jam: 12 sub-bucket @ 5 menit ----
    uint32_t slot = epoch / RAIN_HOUR_BUCKET_SPAN_SEC;
    uint8_t idx = (uint8_t)(slot % RAIN_HOUR_BUCKET_COUNT);
    if (hourBucketSlot[idx] != slot) {
        hourBucket[idx] = 0.0f;
        hourBucketSlot[idx] = slot;
    }
    hourBucket[idx] += deltaMm;

    // ---- 24 jam / minggu / bulan: reset periodik dari titik acuan ----
    if (rain24hStart == 0 || epoch >= rain24hStart + SECONDS_PER_DAY) {
        rain24h = 0.0f;
        rain24hStart = epoch;
    }
    rain24h += deltaMm;

    if (rainWeekStart == 0 || epoch >= rainWeekStart + SECONDS_PER_WEEK) {
        rainWeek = 0.0f;
        rainWeekStart = epoch;
    }
    rainWeek += deltaMm;

    if (rainMonthStart == 0 || epoch >= rainMonthStart + SECONDS_PER_MONTH) {
        rainMonth = 0.0f;
        rainMonthStart = epoch;
    }
    rainMonth += deltaMm;
}

float WeatherDecoder::sumRollingHour(uint32_t epoch) {
    uint32_t nowSlot = epoch / RAIN_HOUR_BUCKET_SPAN_SEC;
    float total = 0.0f;
    for (uint8_t i = 0; i < RAIN_HOUR_BUCKET_COUNT; i++) {
        if (hourBucketSlot[i] == 0) continue;
        uint32_t age = nowSlot - hourBucketSlot[i];
        if (age >= RAIN_HOUR_BUCKET_COUNT) {
            hourBucket[i] = 0.0f;
            continue;
        }
        total += hourBucket[i];
    }
    return total;
}

void WeatherDecoder::resetRainCounter(uint16_t initialCounter) {
    rainCounterPrev = initialCounter;
    rainAccumulated = 0.0f;
}

void WeatherDecoder::loadRainState() {
    // Dipanggil SEKALI di SystemManager::init() -- di mode kontinu
    // decoder ini hidup sepanjang runtime (bukan dibuat ulang tiap
    // "wake" seperti arsitektur lama), jadi cukup dipulihkan dari NVS
    // satu kali di awal, lalu state di-update terus di RAM selama
    // device menyala.
    rainCounterPrev = PersistentState::getRainCounterPrev();
    rainAccumulated = PersistentState::getRainAccumulated();
    for (uint8_t i = 0; i < RAIN_HOUR_BUCKET_COUNT; i++) {
        hourBucket[i] = PersistentState::getRainHourBucket(i);
        hourBucketSlot[i] = PersistentState::getRainHourBucketSlot(i);
    }
    rain24h = PersistentState::getRain24h();
    rain24hStart = PersistentState::getRain24hStartEpoch();
    rainWeek = PersistentState::getRainWeek();
    rainWeekStart = PersistentState::getRainWeekStartEpoch();
    rainMonth = PersistentState::getRainMonth();
    rainMonthStart = PersistentState::getRainMonthStartEpoch();
}

void WeatherDecoder::saveRainState() {
    // Dipanggil tiap ada paket baru yang membawa delta rain -- menulis
    // ke NVS (flash) supaya selamat lintas brownout/reboot, TIDAK
    // terkait deep sleep (yang sudah tidak dipakai lagi).
    PersistentState::setRainCounterPrev(rainCounterPrev);
    PersistentState::setRainAccumulated(rainAccumulated);
    for (uint8_t i = 0; i < RAIN_HOUR_BUCKET_COUNT; i++) {
        PersistentState::setRainHourBucket(i, hourBucket[i], hourBucketSlot[i]);
    }
    PersistentState::setRain24h(rain24h, rain24hStart);
    PersistentState::setRainWeek(rainWeek, rainWeekStart);
    PersistentState::setRainMonth(rainMonth, rainMonthStart);
    PersistentState::persistRainStateToNVS();
}

bool WeatherDecoder::decodePacket(uint8_t* packet, uint8_t len, WeatherData& data, float& rainAccumulatedRef, uint32_t epoch) {
    if (len < 10) return false;

    data.sensorId   = packet[1];
    uint16_t rawT   = ((uint16_t)(packet[1] & 0x0F) << 8) | packet[2];
    data.temperature = (rawT - TEMP_OFFSET) / TEMP_DIVISOR;
    data.humidity   = packet[3];
    data.windSpeed  = packet[4] * WIND_FACTOR;
    data.windGust   = packet[5] * WIND_FACTOR;
    uint16_t rainRaw = ((uint16_t)packet[6] << 8) | packet[7];
    data.batteryOk  = (packet[8] & 0x80) != 0;
    data.channel    = (packet[8] >> 4) & 0x07;
    data.windDirection = packet[8] & 0x0F;
    data.windDeg    = data.windDirection * WIND_DEG_STEP;
    data.rainRaw    = rainRaw;
    // data.light TIDAK disentuh di sini: nilainya diisi oleh
    // SystemManager dari pembacaan LightSensor kontinu paling baru
    // sebelum decodePacket() dipanggil.

    static const uint16_t MAX_PLAUSIBLE_RAIN_DIFF_PER_CYCLE = 200; // ~60mm/siklus, sangat generous utk badai ekstrem
    float delta = 0.0f;
    if (rainCounterPrev != RAIN_UNINIT) {
        uint16_t prev = rainCounterPrev;
        if (rainRaw != prev) {
            uint16_t diff = (rainRaw > prev) ? (rainRaw - prev) : (uint16_t)(65536UL - prev + rainRaw);
            if (diff > MAX_PLAUSIBLE_RAIN_DIFF_PER_CYCLE) {
                LOG_WARN("Rain counter meloncat tidak wajar (%u -> %u, diff=%u), diabaikan -- "
                          "kemungkinan paket rusak lolos CRC", prev, rainRaw, diff);
                data.rainRaw = prev;
                data.rainDelta = sumRollingHour(epoch);
                data.rainTotal = rainAccumulated;
                rainAccumulatedRef = rainAccumulated;
                return true;
            }
            delta = diff * RAIN_MM_PER_TIP;
            rainAccumulated += delta;
        }
    }
    rainCounterPrev = rainRaw;
    if (delta > 0.0f) {
        addToTieredAccumulators(delta, epoch);
    }
    data.rainDelta = sumRollingHour(epoch);
    data.rainTotal = rainAccumulated;
    rainAccumulatedRef = rainAccumulated;
    return true;
}
