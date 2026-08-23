#include "headers.h"

uint32_t PersistentState::bootCount = 0;

WeatherData PersistentState::pending[PENDING_BUFFER_SIZE];
uint8_t PersistentState::pendingLen = 0;

uint16_t PersistentState::rainCounterPrev = RAIN_UNINIT;
float PersistentState::rainAccumulated = 0.0f;
float PersistentState::rainHourBucket[RAIN_HOUR_BUCKET_COUNT] = {0};
uint32_t PersistentState::rainHourBucketSlot[RAIN_HOUR_BUCKET_COUNT] = {0};
float PersistentState::rain24h = 0.0f;
uint32_t PersistentState::rain24hStartEpoch = 0;
float PersistentState::rainWeek = 0.0f;
uint32_t PersistentState::rainWeekStartEpoch = 0;
float PersistentState::rainMonth = 0.0f;
uint32_t PersistentState::rainMonthStartEpoch = 0;

void PersistentState::init() {
    // Tidak ada lagi RTC_DATA_ATTR / deep sleep -- setiap kali init()
    // dipanggil artinya ini memang boot baru (power-on atau reset),
    // jadi seluruh state sesi (buffer pending, dst.) mulai dari nol.
    Preferences p;
    if (p.begin("bootcnt", false)) {
        bootCount = p.getUInt("count", 0) + 1;
        p.putUInt("count", bootCount);
        p.end();
    } else {
        bootCount = 0;
    }

    pendingLen = 0;

    // Rain state DIPULIHKAN dari NVS (bukan direset ke 0) -- ini
    // pengukuran jangka panjang yang harus selamat lintas
    // brownout/reboot, sama seperti alasan di arsitektur lama. Bedanya
    // sekarang dipulihkan sekali saja di sini (boot pertama & satu-
    // satunya di mode kontinu), bukan tiap "wake".
    Preferences rp;
    if (rp.begin("rainst", true)) { // read-only
        bool has = rp.isKey("counter");
        uint16_t savedCounter = rp.getUShort("counter", RAIN_UNINIT);
        float savedAccum = rp.getFloat("accum", 0.0f);
        float d24 = rp.getFloat("d24", 0.0f);
        uint32_t d24Start = rp.getUInt("d24s", 0);
        float dWeek = rp.getFloat("dweek", 0.0f);
        uint32_t dWeekStart = rp.getUInt("dweeks", 0);
        float dMonth = rp.getFloat("dmonth", 0.0f);
        uint32_t dMonthStart = rp.getUInt("dmonths", 0);
        rp.end();
        rainCounterPrev = has ? savedCounter : RAIN_UNINIT;
        rainAccumulated = has ? savedAccum : 0.0f;
        rain24h = has ? d24 : 0.0f;
        rain24hStartEpoch = has ? d24Start : 0;
        rainWeek = has ? dWeek : 0.0f;
        rainWeekStartEpoch = has ? dWeekStart : 0;
        rainMonth = has ? dMonth : 0.0f;
        rainMonthStartEpoch = has ? dMonthStart : 0;
    } else {
        rainCounterPrev = RAIN_UNINIT;
        rainAccumulated = 0.0f;
        rain24h = 0.0f;
        rain24hStartEpoch = 0;
        rainWeek = 0.0f;
        rainWeekStartEpoch = 0;
        rainMonth = 0.0f;
        rainMonthStartEpoch = 0;
    }

    // Bucket rolling 1 jam sengaja tidak dipulihkan dari NVS -- state
    // jangka pendek, wajar dibangun ulang dari nol tiap cold-boot.
    for (uint8_t i = 0; i < RAIN_HOUR_BUCKET_COUNT; i++) {
        rainHourBucket[i] = 0.0f;
        rainHourBucketSlot[i] = 0;
    }
}

uint32_t PersistentState::getBootCount() {
    return bootCount;
}

// ---------------- buffer pending upload ----------------

bool PersistentState::pushPending(const WeatherData& data) {
    if (pendingLen >= PENDING_BUFFER_SIZE) return false;
    pending[pendingLen++] = data;
    return true;
}

uint8_t PersistentState::pendingCount() {
    return pendingLen;
}

bool PersistentState::getPending(uint8_t index, WeatherData& out) {
    if (index >= pendingLen) return false;
    out = pending[index];
    return true;
}

void PersistentState::clearPending() {
    pendingLen = 0;
}

void PersistentState::compactPending(const bool keep[], uint8_t count) {
    if (count > pendingLen) count = pendingLen;
    uint8_t w = 0;
    for (uint8_t i = 0; i < count; i++) {
        if (keep[i]) {
            if (w != i) pending[w] = pending[i];
            w++;
        }
    }
    for (uint8_t i = count; i < pendingLen; i++) {
        pending[w++] = pending[i];
    }
    pendingLen = w;
}

// ---------------- state decoder curah hujan ----------------

uint16_t PersistentState::getRainCounterPrev() { return rainCounterPrev; }
void PersistentState::setRainCounterPrev(uint16_t v) { rainCounterPrev = v; }
float PersistentState::getRainAccumulated() { return rainAccumulated; }
void PersistentState::setRainAccumulated(float v) { rainAccumulated = v; }

void PersistentState::persistRainStateToNVS() {
    // Write-through ke NVS (flash) supaya rain state tidak hilang kalau
    // device sempat brownout/mati listrik sesaat. Dipanggil dari
    // WeatherDecoder hanya saat memang ada delta baru, supaya tidak
    // menghabiskan write-cycle flash secara sia-sia.
    Preferences p;
    if (!p.begin("rainst", false)) return;
    p.putUShort("counter", rainCounterPrev);
    p.putFloat("accum", rainAccumulated);
    p.putFloat("d24", rain24h);
    p.putUInt("d24s", rain24hStartEpoch);
    p.putFloat("dweek", rainWeek);
    p.putUInt("dweeks", rainWeekStartEpoch);
    p.putFloat("dmonth", rainMonth);
    p.putUInt("dmonths", rainMonthStartEpoch);
    p.end();
}

// ---------------- akumulator rain bertingkat ----------------

float PersistentState::getRainHourBucket(uint8_t i) {
    if (i >= RAIN_HOUR_BUCKET_COUNT) return 0.0f;
    return rainHourBucket[i];
}

uint32_t PersistentState::getRainHourBucketSlot(uint8_t i) {
    if (i >= RAIN_HOUR_BUCKET_COUNT) return 0;
    return rainHourBucketSlot[i];
}

void PersistentState::setRainHourBucket(uint8_t i, float mm, uint32_t slot) {
    if (i >= RAIN_HOUR_BUCKET_COUNT) return;
    rainHourBucket[i] = mm;
    rainHourBucketSlot[i] = slot;
}

float PersistentState::getRain24h() { return rain24h; }
uint32_t PersistentState::getRain24hStartEpoch() { return rain24hStartEpoch; }
void PersistentState::setRain24h(float mm, uint32_t startEpoch) {
    rain24h = mm;
    rain24hStartEpoch = startEpoch;
}

float PersistentState::getRainWeek() { return rainWeek; }
uint32_t PersistentState::getRainWeekStartEpoch() { return rainWeekStartEpoch; }
void PersistentState::setRainWeek(float mm, uint32_t startEpoch) {
    rainWeek = mm;
    rainWeekStartEpoch = startEpoch;
}

float PersistentState::getRainMonth() { return rainMonth; }
uint32_t PersistentState::getRainMonthStartEpoch() { return rainMonthStartEpoch; }
void PersistentState::setRainMonth(float mm, uint32_t startEpoch) {
    rainMonth = mm;
    rainMonthStartEpoch = startEpoch;
}
