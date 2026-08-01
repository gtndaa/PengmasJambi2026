#include "headers.h"

WeatherData SystemManager::lastWeather;
DeviceStatus SystemManager::status;
uint32_t SystemManager::lastUploadTime = 0;
bool SystemManager::packetReceivedThisCycle = false;

// State internal untuk siklus berjalan (tidak perlu di RTC memory karena
// hanya dipakai dalam satu eksekusi run() -> computeNextSleepMs()).
static bool s_rtcOk = false;

// =====================================================================
// CATATAN ARSITEKTUR DUTY-CYCLE
// =====================================================================
// Dari analisis log paket radio sensor cuaca (id 0x22 ch4), transmisi
// terjadi setiap ~48 detik (TX_PERIOD_MS) dengan jitter kecil dan sesekali
// paket hilang. Karena ESP32 deep sleep = reset total (bukan loop yang
// tertidur), strategi hemat dayanya adalah:
//
//   1. init()  -> inisialisasi minimal, baca state dari RTC memory
//   2. run()   -> SATU siklus kerja: dengar radio, baca lux, (mungkin)
//                 upload ke cloud, lalu hitung durasi sleep berikutnya
//   3. Scheduler::goToSleep() -> deep sleep sampai jadwal paket berikutnya
//
// Saat belum tahu jadwal sensor (first boot / re-sync setelah banyak
// paket hilang), ESP32 mendengar penuh LISTEN_WINDOW_MS (30s) untuk
// menangkap 1 paket dan mengunci sinkronisasi ke RTC (via
// RTCMemory::setLastPacketEpoch). Setelah sinkron, ESP32 cukup bangun
// sesaat sebelum jadwal prediksi (guard window TX_JITTER_GUARD_MS*2),
// jauh lebih hemat daripada mendengar terus-menerus.
// =====================================================================

void SystemManager::init() {
    BootManager::init();
    Logger::setOutput(&Serial);
    Serial.begin(115200);
    delay(200);

    RTCMemory::incrementWakeCounter();
    LOG_INFO("=== Wake #%lu (boot #%lu) ===",
             (unsigned long)RTCMemory::getWakeCounter(),
             (unsigned long)RTCMemory::getBootCount());

    Wire.begin(I2C_SDA, I2C_SCL);

    status.radioPresent = false;
    status.rtcPresent = false;
    status.lightSensorPresent = false;
    status.sdCardPresent = false;

    PowerManager pm;
    pm.begin();
}

void SystemManager::run() {
    packetReceivedThisCycle = false;

    RTCManager rtc;
    s_rtcOk = rtc.begin() && rtc.isOK();
    status.rtcPresent = s_rtcOk;

    CC1101Driver radio;
    status.radioPresent = radio.begin();
    if (!status.radioPresent) {
        LOG_ERROR("CC1101 gagal inisialisasi, siklus ini lewati mode sinkron radio");
    }

    LightSensor light;
    status.lightSensorPresent = light.begin();

    // Tentukan durasi window dengar radio berdasarkan status sinkronisasi.
    uint32_t listenMs;
    if (s_rtcOk && RTCMemory::isSynced()) {
        // Sudah tahu jadwal transmisi -> cukup dengar sebentar di sekitar
        // waktu prediksi (guard window mengakomodasi jitter timing sensor).
        listenMs = TX_JITTER_GUARD_MS * 2;
        LOG_INFO("Mode sinkron (missed=%d): dengar %lu ms",
                 RTCMemory::getMissedCycles(), (unsigned long)listenMs);
    } else {
        // Belum sinkron / RTC tidak tersedia / terlalu banyak paket
        // hilang berturut-turut -> dengar penuh 1 periode transmisi
        // untuk menangkap paket pertama dan membangun ulang sinkronisasi.
        listenMs = LISTEN_WINDOW_MS;
        LOG_INFO("Mode sinkronisasi ulang: dengar penuh %lu ms", (unsigned long)listenMs);
    }

    if (status.radioPresent) {
        receiveWeather(listenMs);
    }

    readLight();

    // Simpan salinan lokal ke SD sebagai backup (opsional, tidak
    // menggagalkan siklus jika SD tidak ada).
    storeData();

    // Upload hanya setiap beberapa siklus (mengikuti UPLOAD_INTERVAL_MS
    // yang dikonversi ke kelipatan TX_PERIOD_MS), atau lebih awal jika
    // buffer RTC memory sudah penuh, supaya WiFi tidak dinyalakan tiap 48s.
    uint32_t wake = RTCMemory::getWakeCounter();
    bool bufferFull = (RTCMemory::pendingCount() >= PENDING_BUFFER_SIZE);
    bool scheduledUpload = (wake % UPLOAD_EVERY_N_CYCLES == 0);
    if ((scheduledUpload || bufferFull) && RTCMemory::pendingCount() > 0) {
        upload();
    }

    status.freeHeap = ESP.getFreeHeap();
    status.uptime = millis() / 1000;
    status.bootCount = RTCMemory::getBootCount();
    status.wakeCounter = wake;
    strncpy(status.firmwareVersion, FW_VERSION, sizeof(status.firmwareVersion) - 1);
    status.firmwareVersion[sizeof(status.firmwareVersion) - 1] = '\0';

    PowerManager pm;
    status.batteryVoltage = pm.readBatteryVoltage();
    status.superCapVoltage = pm.readSuperCapVoltage();
}

void SystemManager::receiveWeather(uint32_t listenWindowMs) {
    CC1101Driver radio;
    WeatherDecoder decoder;
    decoder.loadRainStateFromRTC();   // pulihkan akumulasi hujan lintas deep-sleep

    uint32_t pulses[MAX_PULSES];
    uint8_t bits[MAX_PULSES];

    radio.setReceiveMode();
    radio.resetPulseBuffer();
    uint32_t start = millis();
    bool got = false;

    while (millis() - start < listenWindowMs) {
        if (radio.isPacketAvailable()) {
            uint16_t count = radio.getPulseCount();
            radio.copyPulses(pulses, count);
            radio.resetPulseBuffer();

            uint16_t bitCount = decoder.pulsesToBits(pulses, count, bits, MAX_PULSES);
            bool decoded = false;
            if (bitCount >= 64) {
                uint8_t packet[MAX_BYTES];
                uint8_t pktLen;
                if (decoder.scanForPacket(bits, bitCount, packet, &pktLen)) {
                    float rainAcc;
                    if (decoder.decodePacket(packet, pktLen, lastWeather, rainAcc)) {
                        RTCManager rtc;
                        uint32_t epoch;
                        if (rtc.begin() && rtc.isOK()) {
                            epoch = rtc.now().unixtime();
                        } else {
                            epoch = millis() / 1000; // fallback tanpa RTC (tidak ideal utk sync)
                        }
                        lastWeather.timestamp = epoch;

                        decoder.saveRainStateToRTC();

                        RTCMemory::setLastPacketEpoch(epoch);
                        RTCMemory::resetMissedCycles();
                        if (!RTCMemory::pushPending(lastWeather)) {
                            LOG_WARN("Buffer pending penuh, data ini akan hilang jika belum diupload");
                        }

                        LOG_INFO("Paket diterima: T=%.1fC H=%d%% Wind=%.1fkm/h Rain=%.1fmm Batt=%s",
                                  lastWeather.temperature, lastWeather.humidity,
                                  lastWeather.windSpeed, lastWeather.rainTotal,
                                  lastWeather.batteryOk ? "OK" : "LOW");

                        packetReceivedThisCycle = true;
                        got = true;
                        decoded = true;
                    }
                }
            }

            if (!decoded) {
                // Kemungkinan noise / paket rusak (mis. "Paket terlalu pendek")
                // -> lanjut dengar sisa window, jangan langsung menyerah.
                radio.setReceiveMode();
                if (millis() - start < listenWindowMs) continue;
            }
            break;
        }
        delay(10);
    }

    if (!got) {
        RTCMemory::incrementMissedCycles();
        LOG_WARN("Tidak ada paket valid dalam window %lu ms (missed berturut-turut=%d)",
                 (unsigned long)listenWindowMs, RTCMemory::getMissedCycles());
    }

    radio.watchdogReset();
}

void SystemManager::readLight() {
    LightSensor light;
    if (light.begin()) {
        lastWeather.light = light.readOnce();
    }
}

void SystemManager::storeData() {
    SDManager sd;
    if (!sd.begin() || !sd.isPresent()) return;
    status.sdCardPresent = true;
    String line = String(lastWeather.timestamp) + ","
                + String(lastWeather.temperature) + ","
                + String(lastWeather.humidity) + ","
                + String(lastWeather.windSpeed) + ","
                + String(lastWeather.rainTotal) + ","
                + String(lastWeather.light);
    sd.appendRecord(line.c_str());
}

void SystemManager::upload() {
    uint8_t count = RTCMemory::pendingCount();
    if (count == 0) return;

    DeviceConfig cfg;
    ConfigManager config;
    config.begin();
    config.load(cfg);

    WifiManager wifi;
    if (!wifi.connect(cfg.wifiSSID.c_str(), cfg.wifiPassword.c_str())) {
        LOG_ERROR("Gagal konek WiFi, %d data tetap tersimpan di RTC memory untuk dicoba lagi", count);
        return;
    }

    WiFiClient espClient;
    CloudAPI api(espClient);
    api.begin(cfg.serverURL.c_str(), cfg.apiKey.c_str(),
              cfg.mqttBroker.c_str(), cfg.mqttPort,
              cfg.mqttClientId.c_str(),
              cfg.mqttUsername.c_str(), cfg.mqttPassword.c_str());

    if (!cfg.mqttBroker.isEmpty()) {
        api.connectMQTT();
    }

    uint8_t sent = 0;
    for (uint8_t i = 0; i < count; i++) {
        WeatherData d;
        if (!RTCMemory::getPending(i, d)) continue;
        if (api.uploadWeather(d)) sent++;
    }

    status.wifiRSSI = (int8_t)wifi.getRSSI();
    api.uploadStatus(status);

    if (sent == count) {
        RTCMemory::clearPending();
        LOG_INFO("Upload sukses: %d data terkirim, buffer RTC dikosongkan", sent);
    } else {
        LOG_WARN("Upload sebagian: %d/%d terkirim, sisanya tetap di buffer", sent, count);
    }

    wifi.disconnect();
}

uint32_t SystemManager::computeNextSleepMs() {
    // Belum sinkron atau RTC bermasalah -> pakai fallback interval tetap,
    // dan biarkan siklus berikutnya mencoba sinkronisasi ulang.
    if (!s_rtcOk || !RTCMemory::isSynced()) {
        LOG_INFO("Belum sinkron, sleep fallback %lu ms", (unsigned long)SLEEP_INTERVAL_MS);
        return SLEEP_INTERVAL_MS;
    }

    RTCManager rtc;
    if (!rtc.begin() || !rtc.isOK()) {
        return SLEEP_INTERVAL_MS;
    }

    uint32_t now = rtc.now().unixtime();
    uint32_t lastPkt = RTCMemory::getLastPacketEpoch();
    uint32_t periodSec = TX_PERIOD_MS / 1000UL;

    // Bulatkan maju ke siklus TX_PERIOD_MS berikutnya dari referensi
    // paket valid terakhir, sehingga tetap presisi walau ada beberapa
    // siklus yang meleset (missed) dari jadwal.
    uint32_t elapsed = (now >= lastPkt) ? (now - lastPkt) : 0;
    uint32_t cyclesPassed = elapsed / periodSec;
    uint32_t nextExpected = lastPkt + (cyclesPassed + 1) * periodSec;

    int64_t sleepSec = (int64_t)nextExpected - (int64_t)now - (int64_t)(WAKE_BEFORE_MS / 1000UL);
    if (sleepSec < 0) sleepSec = 0;

    uint32_t sleepMs = (uint32_t)sleepSec * 1000UL;
    if (sleepMs < MIN_SLEEP_MS) sleepMs = MIN_SLEEP_MS;

    LOG_INFO("Prediksi paket berikutnya ~%ld s lagi -> sleep %lu ms",
              (long)sleepSec, (unsigned long)sleepMs);
    return sleepMs;
}
