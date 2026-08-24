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

        // PENTING: rainCounterPrev & rainAccumulated SENGAJA TIDAK
        // direset ke UNINIT/0 di sini seperti field lain di atas.
        // Field-field lain itu memang state SESI/jadwal yang wajar
        // reset tiap cold-boot -- tapi akumulasi hujan itu pengukuran
        // JANGKA PANJANG yang seharusnya tidak boleh hilang cuma
        // karena device sempat brownout/restart sesaat (device ini
        // sudah beberapa kali mengalami POWERON_RESET & RTC
        // lostPower() akibat tegangan kurang stabil). RTC memory
        // sendiri tidak bisa bertahan lintas cold-boot (itu sifat
        // dasarnya), jadi rain state dipulihkan dari NVS/flash
        // (Preferences) yang memang didesain untuk bertahan lintas
        // power loss. Kalau tidak ada data NVS sama sekali (device
        // benar-benar baru pertama kali nyala), baru fallback ke
        // UNINIT/0.
        Preferences p;
        if (p.begin("rainst", true)) { // read-only
            bool has = p.isKey("counter");
            uint16_t savedCounter = p.getUShort("counter", RAIN_UNINIT);
            float savedAccum = p.getFloat("accum", 0.0f);
            float d24 = p.getFloat("d24", 0.0f);
            uint32_t d24Start = p.getUInt("d24s", 0);
            float dWeek = p.getFloat("dweek", 0.0f);
            uint32_t dWeekStart = p.getUInt("dweeks", 0);
            float dMonth = p.getFloat("dmonth", 0.0f);
            uint32_t dMonthStart = p.getUInt("dmonths", 0);
            p.end();
            rtcData->rainCounterPrev = has ? savedCounter : RAIN_UNINIT;
            rtcData->rainAccumulated = has ? savedAccum : 0.0f;
            // 24h/week/month juga dipulihkan dari NVS dengan alasan sama:
            // kehilangan progres sehari/seminggu/sebulan gara-gara device
            // sempat brownout/restart itu jauh lebih mengganggu dibanding
            // bucket 1 jam (yang wajar dibangun ulang dalam <=1 jam saja).
            rtcData->rain24h = has ? d24 : 0.0f;
            rtcData->rain24hStartEpoch = has ? d24Start : 0;
            rtcData->rainWeek = has ? dWeek : 0.0f;
            rtcData->rainWeekStartEpoch = has ? dWeekStart : 0;
            rtcData->rainMonth = has ? dMonth : 0.0f;
            rtcData->rainMonthStartEpoch = has ? dMonthStart : 0;
        } else {
            rtcData->rainCounterPrev = RAIN_UNINIT;
            rtcData->rainAccumulated = 0.0f;
            rtcData->rain24h = 0.0f;
            rtcData->rain24hStartEpoch = 0;
            rtcData->rainWeek = 0.0f;
            rtcData->rainWeekStartEpoch = 0;
            rtcData->rainMonth = 0.0f;
            rtcData->rainMonthStartEpoch = 0;
        }

        // Bucket rolling 1 jam SENGAJA tidak dipulihkan dari NVS -- ini
        // state jangka pendek (maks 1 jam), wajar dibangun ulang dari
        // nol pasca cold-boot tanpa perlu menulis ke flash tiap kali
        // (menghindari write-cycle NVS yang sia-sia untuk data yang
        // umurnya cuma hitungan menit).
        for (uint8_t i = 0; i < RAIN_HOUR_BUCKET_COUNT; i++) {
            rtcData->rainHourBucket[i] = 0.0f;
            rtcData->rainHourBucketSlot[i] = 0;
        }
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
    // Menyusun ulang buffer supaya hanya menyisakan entri yang GAGAL
    // dipindah/dikirim (keep[i]==true), digeser rapat ke depan. Dipakai
    // supaya data yang gagal disimpan ke SD (mis. SPI/SD lagi bentrok
    // dengan radio) TIDAK ikut hilang -- tetap bertahan di RTC memory
    // untuk dicoba lagi siklus berikutnya, bukan langsung dibuang
    // seperti clearPending() tanpa syarat.
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

uint16_t RTCMemory::getRainCounterPrev() {
    return rtcData->rainCounterPrev;
}

void RTCMemory::setRainCounterPrev(uint16_t v) {
    rtcData->rainCounterPrev = v;
}

float RTCMemory::getRainAccumulated() {
    return rtcData->rainAccumulated;
}

void RTCMemory::setRainAccumulated(float v) {
    rtcData->rainAccumulated = v;
}

void RTCMemory::persistRainStateToNVS() {
    // Write-through ke NVS supaya rain state selamat lintas cold-boot
    // (lihat penjelasan panjang di init()). Sengaja TIDAK dipanggil di
    // setiap wake -- cukup dipanggil dari WeatherDecoder setelah paket
    // valid diproses (jadi hanya nulis flash ketika memang ada data
    // baru), supaya tidak menghabiskan write-cycle NVS/flash secara
    // sia-sia di siklus yang tidak menangkap paket sama sekali.
    Preferences p;
    if (!p.begin("rainst", false)) return; // read-write
    p.putUShort("counter", rtcData->rainCounterPrev);
    p.putFloat("accum", rtcData->rainAccumulated);
    p.putFloat("d24", rtcData->rain24h);
    p.putUInt("d24s", rtcData->rain24hStartEpoch);
    p.putFloat("dweek", rtcData->rainWeek);
    p.putUInt("dweeks", rtcData->rainWeekStartEpoch);
    p.putFloat("dmonth", rtcData->rainMonth);
    p.putUInt("dmonths", rtcData->rainMonthStartEpoch);
    p.end();
}

// ---------------- akumulator rain bertingkat ----------------

float RTCMemory::getRainHourBucket(uint8_t i) {
    if (i >= RAIN_HOUR_BUCKET_COUNT) return 0.0f;
    return rtcData->rainHourBucket[i];
}

uint32_t RTCMemory::getRainHourBucketSlot(uint8_t i) {
    if (i >= RAIN_HOUR_BUCKET_COUNT) return 0;
    return rtcData->rainHourBucketSlot[i];
}

void RTCMemory::setRainHourBucket(uint8_t i, float mm, uint32_t slot) {
    if (i >= RAIN_HOUR_BUCKET_COUNT) return;
    rtcData->rainHourBucket[i] = mm;
    rtcData->rainHourBucketSlot[i] = slot;
}

float RTCMemory::getRain24h() { return rtcData->rain24h; }
uint32_t RTCMemory::getRain24hStartEpoch() { return rtcData->rain24hStartEpoch; }
void RTCMemory::setRain24h(float mm, uint32_t startEpoch) {
    rtcData->rain24h = mm;
    rtcData->rain24hStartEpoch = startEpoch;
}

float RTCMemory::getRainWeek() { return rtcData->rainWeek; }
uint32_t RTCMemory::getRainWeekStartEpoch() { return rtcData->rainWeekStartEpoch; }
void RTCMemory::setRainWeek(float mm, uint32_t startEpoch) {
    rtcData->rainWeek = mm;
    rtcData->rainWeekStartEpoch = startEpoch;
}

float RTCMemory::getRainMonth() { return rtcData->rainMonth; }
uint32_t RTCMemory::getRainMonthStartEpoch() { return rtcData->rainMonthStartEpoch; }
void RTCMemory::setRainMonth(float mm, uint32_t startEpoch) {
    rtcData->rainMonth = mm;
    rtcData->rainMonthStartEpoch = startEpoch;
}
