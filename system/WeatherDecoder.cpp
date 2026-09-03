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
        // Sub-bucket ini mewakili rentang 5-menit yang beda dari
        // terakhir kali dipakai (baru pertama kali, atau sudah muter
        // lagi setelah >1 jam) -> mulai dari 0 dulu.
        hourBucket[idx] = 0.0f;
        hourBucketSlot[idx] = slot;
    }
    hourBucket[idx] += deltaMm;

    // ---- 24 jam / minggu / bulan: reset periodik dari titik acuan ----
    // Catatan: ini BUKAN kalender asli (tidak selalu pas jam 00:00 /
    // Senin / tanggal 1), tapi reset tiap genap 24 jam/7 hari/30 hari
    // sejak titik acuan terakhir -- cukup untuk keperluan pemantauan
    // tanpa perlu dekomposisi kalender penuh di firmware.
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
        if (hourBucketSlot[i] == 0) continue; // belum pernah dipakai
        uint32_t age = nowSlot - hourBucketSlot[i]; // unsigned, aman kalau underflow (jadi besar -> dianggap basi)
        if (age >= RAIN_HOUR_BUCKET_COUNT) {
            // Sudah >60 menit sejak sub-bucket ini terakhir dipakai
            // (device lama tidak menangkap paket) -> basi, nolkan.
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

void WeatherDecoder::loadRainStateFromRTC() {
    // Karena tiap siklus bangun adalah "boot" baru bagi objek ini,
    // state akumulasi hujan harus dipulihkan dari RTC memory supaya
    // tidak reset ke 0 setiap kali deep sleep.
    rainCounterPrev = RTCMemory::getRainCounterPrev();
    rainAccumulated = RTCMemory::getRainAccumulated();
    for (uint8_t i = 0; i < RAIN_HOUR_BUCKET_COUNT; i++) {
        hourBucket[i] = RTCMemory::getRainHourBucket(i);
        hourBucketSlot[i] = RTCMemory::getRainHourBucketSlot(i);
    }
    rain24h = RTCMemory::getRain24h();
    rain24hStart = RTCMemory::getRain24hStartEpoch();
    rainWeek = RTCMemory::getRainWeek();
    rainWeekStart = RTCMemory::getRainWeekStartEpoch();
    rainMonth = RTCMemory::getRainMonth();
    rainMonthStart = RTCMemory::getRainMonthStartEpoch();
}

void WeatherDecoder::saveRainStateToRTC() {
    RTCMemory::setRainCounterPrev(rainCounterPrev);
    RTCMemory::setRainAccumulated(rainAccumulated);
    for (uint8_t i = 0; i < RAIN_HOUR_BUCKET_COUNT; i++) {
        RTCMemory::setRainHourBucket(i, hourBucket[i], hourBucketSlot[i]);
    }
    RTCMemory::setRain24h(rain24h, rain24hStart);
    RTCMemory::setRainWeek(rainWeek, rainWeekStart);
    RTCMemory::setRainMonth(rainMonth, rainMonthStart);
    // Write-through ke NVS (flash) supaya rain state tidak hilang kalau
    // device sempat cold-boot (mati listrik sesaat, brownout, dsb)
    // beda dengan RTC memory yang otomatis terhapus di kondisi itu.
    RTCMemory::persistRainStateToNVS();
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

    static const uint16_t MAX_PLAUSIBLE_RAIN_DIFF_PER_CYCLE = 200; // ~60mm/siklus 48s, sangat generous utk badai ekstrem
    float delta = 0.0f;
    if (rainCounterPrev != RAIN_UNINIT) {
        uint16_t prev = rainCounterPrev;
        if (rainRaw != prev) {
            uint16_t diff = (rainRaw > prev) ? (rainRaw - prev) : (uint16_t)(65536UL - prev + rainRaw);
            if (diff > MAX_PLAUSIBLE_RAIN_DIFF_PER_CYCLE) {
                LOG_WARN("Rain counter meloncat tidak wajar (%u -> %u, diff=%u), diabaikan -- "
                          "kemungkinan paket rusak lolos CRC", prev, rainRaw, diff);
                data.rainRaw = prev; // laporkan counter LAMA yang masih dipercaya, bukan yang rusak
                data.rainDelta = sumRollingHour(epoch); // tetap kirim rolling 1h yang valid, cuma tidak nambah apa-apa
                data.rainTotal = rainAccumulated;
                rainAccumulatedRef = rainAccumulated;
                return true; // field lain di paket ini (suhu dkk) tetap valid & dipakai
            }
            delta = diff * RAIN_MM_PER_TIP;
            rainAccumulated += delta;
        }
    }
    rainCounterPrev = rainRaw;
    if (delta > 0.0f) {
        addToTieredAccumulators(delta, epoch);
    }
    // rain_delta yang dikirim ke server = akumulasi ROLLING 1 JAM terakhir
    data.rainDelta = sumRollingHour(epoch);
    data.rainTotal = rainAccumulated;
    rainAccumulatedRef = rainAccumulated;
    return true;
}