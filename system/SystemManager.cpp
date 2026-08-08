#include "headers.h"

WeatherData SystemManager::lastWeather;
DeviceStatus SystemManager::status;
bool SystemManager::packetReceivedThisCycle = false;

// State internal untuk siklus berjalan (tidak perlu di RTC memory karena
// hanya dipakai dalam satu eksekusi run() -> computeNextSleepMs()).
static bool s_rtcOk = false;

// =====================================================================
// CATATAN ARSITEKTUR DUTY-CYCLE
// =====================================================================
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
// sesaat sebelum jadwal prediksi (guard window TX_JITTER_GUARD_MS*2)
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

uint32_t SystemManager::predictNextExpectedEpoch(uint32_t lastPkt, uint32_t now) {
    uint32_t periodSec = TX_PERIOD_MS / 1000UL;
    uint32_t elapsed = (now >= lastPkt) ? (now - lastPkt) : 0;
    uint32_t cyclesPassed = elapsed / periodSec;
    return lastPkt + (cyclesPassed + 1) * periodSec;
}

void SystemManager::run() {
    packetReceivedThisCycle = false;

    DeviceConfig cfg;
    ConfigManager config;
    config.begin();
    config.load(cfg);

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
        uint32_t nowEpoch = rtc.now().unixtime();
        uint32_t lastPkt = RTCMemory::getLastPacketEpoch();
        uint32_t nextExpected = predictNextExpectedEpoch(lastPkt, nowEpoch);
        uint32_t windowEndEpoch = nextExpected + (TX_JITTER_GUARD_MS / 1000UL) + 1;

        int64_t remainMs = (int64_t)(windowEndEpoch - nowEpoch) * 1000LL;
        listenMs = (remainMs > (int64_t)MIN_SLEEP_MS) ? (uint32_t)remainMs : MIN_SLEEP_MS;
        LOG_INFO("Mode sinkron (missed=%d): target habis %lu s lagi -> dengar %lu ms",
                 RTCMemory::getMissedCycles(), (unsigned long)(windowEndEpoch - nowEpoch),
                 (unsigned long)listenMs);
    } else {
        listenMs = cfg.listenWindow;
        LOG_INFO("Mode sinkronisasi ulang: dengar penuh %lu ms", (unsigned long)listenMs);
    }

    if (status.radioPresent) {
        receiveWeather(listenMs);
    }

    readLight(light);

    if (packetReceivedThisCycle) {
        if (!RTCMemory::pushPending(lastWeather)) {
            LOG_WARN("Buffer pending penuh, data ini akan hilang jika belum diupload");
        }
    }

    // Simpan salinan lokal ke SD sebagai backup (opsional, tidak
    // menggagalkan siklus jika SD tidak ada).
    storeData();

    // Upload hanya setiap beberapa siklus (mengikuti UPLOAD_INTERVAL_MS
    // yang dikonversi ke kelipatan TX_PERIOD_MS), atau lebih awal jika
    // buffer RTC memory sudah penuh, supaya WiFi tidak dinyalakan tiap 48s.
    uint32_t wake = RTCMemory::getWakeCounter();
    bool bufferFull = (RTCMemory::pendingCount() >= PENDING_BUFFER_SIZE);
    bool scheduledUpload = (wake % UPLOAD_EVERY_N_CYCLES == 0);
    if (scheduledUpload || bufferFull) {
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

    // predictedBefore dihitung dari lastPacketEpoch yg belum diupdate
    // siklus ini, jadi ini representasi prediksi yang dipakai untuk
    // memutuskan listenWindowMs di run().
    uint32_t predictedBefore = 0;
    bool havePrediction = false;
    {
        RTCManager rtcDiag;
        if (rtcDiag.begin() && rtcDiag.isOK()) {
            uint32_t nowEpoch = rtcDiag.now().unixtime();
            uint32_t lastPktBefore = RTCMemory::getLastPacketEpoch();
            if (lastPktBefore != 0) {
                predictedBefore = predictNextExpectedEpoch(lastPktBefore, nowEpoch);
                havePrediction = true;
                LOG_INFO("Mulai dengar: now=%lu prediksi=%lu (margin %ld s), window=%lu ms",
                          (unsigned long)nowEpoch, (unsigned long)predictedBefore,
                          (long)predictedBefore - (long)nowEpoch, (unsigned long)listenWindowMs);
            }
        }
    }

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

                        if (havePrediction) {
                            LOG_INFO("Paket ditangkap: epoch=%lu, meleset %ld s dari prediksi",
                                      (unsigned long)epoch, (long)epoch - (long)predictedBefore);
                        }

                        decoder.saveRainStateToRTC();

                        RTCMemory::setLastPacketEpoch(epoch);
                        RTCMemory::resetMissedCycles();

                        LOG_INFO("Paket diterima: T=%.1fC H=%d%% Wind=%.1fkm/h Rain=%.1fmm Batt=%s",
                                  lastWeather.temperature, lastWeather.humidity,
                                  lastWeather.windSpeed, lastWeather.rainTotal,
                                  lastWeather.batteryOk ? "OK" : "LOW");

                        got = true;
                        decoded = true;
                        packetReceivedThisCycle = true;
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

void SystemManager::readLight(LightSensor& light) {
    lastWeather.light = light.readOnce();
    delay(200);
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

    // SD dipakai sebagai penyimpanan PERSISTEN untuk data yang gagal
    // terkirim: beda dengan RTC memory yang hilang kalau ESP32 benar-benar
    // mati/reset total, file di SD tetap ada sampai data itu sukses masuk
    // ke database server.
    SDManager sd;
    bool sdReady = sd.begin() && sd.isPresent();
    status.sdCardPresent = sdReady;

    uint16_t sdBacklogBefore = sdReady ? sd.queueCount() : 0;
    if (count == 0 && sdBacklogBefore == 0) {
        return; // benar-benar tidak ada apa pun untuk dikirim
    }

    DeviceConfig cfg;
    ConfigManager config;
    config.begin();
    config.load(cfg);

    // Konek pakai kredensial yang sudah diketahui berhasil (tersimpan di NVS,
    // default-nya dari secrets.h saat pertama kali boot).
    WifiManager wifi;
    if (!wifi.connect(cfg.wifiSSID.c_str(), cfg.wifiPassword.c_str())) {
        LOG_ERROR("Gagal konek WiFi");
        if (sdReady) {
            // Pindahkan semua data yang baru terkumpul ke SD supaya tidak
            // hilang walau terjadi reset/mati total sebelum sempat konek
            // lagi ke WiFi.
            bool moved_[PENDING_BUFFER_SIZE] = {false};
            uint8_t moved = 0;
            for (uint8_t i = 0; i < count && i < PENDING_BUFFER_SIZE; i++) {
                WeatherData d;
                bool ok = RTCMemory::getPending(i, d) && sd.queuePush(d);
                moved_[i] = ok;
                if (ok) moved++;
            }
            
            bool keep[PENDING_BUFFER_SIZE];
            for (uint8_t i = 0; i < count && i < PENDING_BUFFER_SIZE; i++) keep[i] = !moved_[i];
            RTCMemory::compactPending(keep, count);
            LOG_WARN("%d/%d data dipindah ke SD (gagal WiFi), %d tetap di buffer RTC (SD gagal)",
                      moved, count, count - moved);
        } else {
            LOG_WARN("SD tidak tersedia & WiFi gagal -- %d data tetap di buffer RTC, dicoba lagi siklus berikutnya", count);
        }
        return;
    }

    CloudAPI api;
    api.begin(cfg.serverURL.c_str(), cfg.apiKey.c_str());

    // ------------------------------------------------------------------
    // Sinkronisasi config dari server (GET /config), berbasis configVersion:
    //
    //  - configVersion==0 di lokal artinya belum pernah sinkron sama
    //    sekali (first init) -> hanya catat versi server, JANGAN ubah
    //    config lain dulu.
    //  - Kalau sudah pernah sinkron dan versi server BERBEDA dari yang
    //    tersimpan -> ambil config baru, terapkan tiap field yang beda.
    //  - Kalau versi sama -> tidak perlu apa-apa (skip diam-diam, tidak
    //    log tiap siklus supaya tidak spam).
    //  - WiFi (ssid/password) diperlakukan khusus: dicoba konek dulu via
    //    connectWithFallback(); hanya dipakai permanen kalau berhasil
    //    connect. Kalau gagal, tetap pakai ssid/password lama.
    // ------------------------------------------------------------------
    RemoteConfig remote;
    if (api.fetchRemoteConfig(remote) && remote.hasConfigVersion) {
        if (cfg.configVersion == 0) {
            // First init: cuma catat baseline versi, config lain dibiarkan.
            cfg.configVersion = remote.configVersion;
            config.save(cfg);
            LOG_INFO("Config version awal dicatat: v%u (konfigurasi lain belum diubah)",
                      (unsigned)cfg.configVersion);
        } else if (remote.configVersion != cfg.configVersion) {
            LOG_INFO("Config version server berubah (v%u -> v%u), sinkronisasi...",
                      (unsigned)cfg.configVersion, (unsigned)remote.configVersion);
            bool changed = false;

            // --- WiFi: tes konek dulu sebelum dipakai permanen ---
            bool wifiDifferent = !remote.ssid.isEmpty() &&
                                  ((remote.ssid != cfg.wifiSSID) || (remote.password != cfg.wifiPassword));
            if (wifiDifferent) {
                WifiCredentials newCreds;
                newCreds.ssid = remote.ssid;
                newCreds.password = remote.password;
                bool switched = wifi.connectWithFallback(newCreds,
                                                           cfg.wifiSSID.c_str(),
                                                           cfg.wifiPassword.c_str());
                if (switched) {
                    cfg.wifiSSID = remote.ssid;
                    cfg.wifiPassword = remote.password;
                    changed = true;
                    LOG_INFO("WiFi diganti ke SSID baru: %s", remote.ssid.c_str());
                } else {
                    LOG_WARN("SSID baru (%s) gagal dihubungi, tetap pakai SSID lama: %s",
                              remote.ssid.c_str(), cfg.wifiSSID.c_str());
                }
            }

            // --- Field lain: langsung diterapkan kalau beda (tidak
            //     berisiko memutus konektivitas seperti wifi) ---
            if (remote.uploadInterval != 0 && remote.uploadInterval != cfg.uploadInterval) {
                cfg.uploadInterval = remote.uploadInterval; changed = true;
            }
            if (remote.listenWindow != 0 && remote.listenWindow != cfg.listenWindow) {
                cfg.listenWindow = remote.listenWindow; changed = true;
            }
            if (remote.sleepInterval != 0 && remote.sleepInterval != cfg.sleepInterval) {
                cfg.sleepInterval = remote.sleepInterval; changed = true;
            }
            if (remote.useDeepSleep != cfg.useDeepSleep) {
                cfg.useDeepSleep = remote.useDeepSleep; changed = true;
            }

            cfg.configVersion = remote.configVersion;
            config.save(cfg);
            LOG_INFO("Config version sekarang: v%u (%s)",
                      (unsigned)cfg.configVersion,
                      changed ? "ada perubahan diterapkan" : "tidak ada field lain yang berubah");
        }
    }

    // 1) Kirim data yang baru terkumpul di siklus ini (RTC memory).
    uint8_t sent = 0;
    bool keepInRtc[PENDING_BUFFER_SIZE] = {false};
    for (uint8_t i = 0; i < count && i < PENDING_BUFFER_SIZE; i++) {
        WeatherData d;
        if (!RTCMemory::getPending(i, d)) continue;
        if (api.uploadWeather(d)) {
            sent++;
        } else if (sdReady && sd.queuePush(d)) {
            // Sukses dipindah ke SD -> aman dibuang dari RTC memory.
        } else {
            // Gagal terkirim langsung DAN gagal juga dipindah ke SD
            // (SD tidak ada, atau tulisnya sendiri gagal) -> wajib
            // tetap di RTC memory, jangan sampai hilang.
            keepInRtc[i] = true;
        }
    }
    uint8_t unsent = count - sent;

    // 2) Coba kirim ulang antrian lama di SD (data dari siklus-siklus
    //    sebelumnya yang gagal terkirim). Yang sukses (masuk database)
    //    otomatis dihapus dari SD oleh queueFlush(); yang masih gagal
    //    tetap tersimpan untuk dicoba lagi di interval berikutnya.
    uint16_t sdSent = 0, sdRemaining = 0;
    if (sdReady) {
        // Batasi maks 20 record per siklus upload
        static const uint16_t MAX_QUEUE_FLUSH_PER_CYCLE = 20;
        sd.queueFlush(api, sdSent, sdRemaining, MAX_QUEUE_FLUSH_PER_CYCLE);
        if (sdRemaining > 0) {
            LOG_INFO("Backlog SD masih tersisa %u record, lanjut siklus upload berikutnya", sdRemaining);
        }
    }

    status.wifiRSSI = (int8_t)wifi.getRSSI();
    api.uploadStatus(status);

    if (unsent == 0) {
        RTCMemory::clearPending();
        LOG_INFO("Upload sukses: %d/%d data terkirim langsung", sent, count);
    } else {
        RTCMemory::compactPending(keepInRtc, count);
        uint8_t stillLost = 0;
        for (uint8_t i = 0; i < count && i < PENDING_BUFFER_SIZE; i++) if (keepInRtc[i]) stillLost++;
        if (stillLost > 0) {
            LOG_WARN("Upload sebagian: %d/%d terkirim langsung, %d dipindah ke SD, %d TETAP di RTC (SD juga gagal)",
                      sent, count, (count - sent - stillLost), stillLost);
        } else {
            LOG_WARN("Upload sebagian: %d/%d terkirim langsung, %d dipindah ke SD", sent, count, unsent);
        }
    }

    if (sdSent > 0 || sdRemaining > 0) {
        LOG_INFO("Antrian SD: %u data lama terkirim & dihapus, %u masih tertunda", sdSent, sdRemaining);
    }

    wifi.disconnect();
}

uint32_t SystemManager::computeNextSleepMs() {
    DeviceConfig cfg;
    ConfigManager config;
    config.begin();
    config.load(cfg);

    // Belum sinkron atau RTC bermasalah -> pakai fallback interval yang
    // bisa diatur dari server (cfg.sleepInterval, GET /config), dan
    // biarkan siklus berikutnya mencoba sinkronisasi ulang.
    if (!s_rtcOk || !RTCMemory::isSynced()) {
        LOG_INFO("Belum sinkron, sleep fallback %lu ms", (unsigned long)cfg.sleepInterval);
        return cfg.sleepInterval;
    }

    RTCManager rtc;
    if (!rtc.begin() || !rtc.isOK()) {
        return cfg.sleepInterval;
    }

    uint32_t now = rtc.now().unixtime();
    uint32_t lastPkt = RTCMemory::getLastPacketEpoch();
    uint32_t nextExpected = predictNextExpectedEpoch(lastPkt, now);
    int64_t sleepSec = (int64_t)nextExpected - (int64_t)now - (int64_t)(WAKE_BEFORE_MS / 1000UL);
    if (sleepSec < 0) sleepSec = 0;

    uint32_t sleepMs = (uint32_t)sleepSec * 1000UL;
    if (sleepMs < MIN_SLEEP_MS) sleepMs = MIN_SLEEP_MS;

    LOG_INFO("Prediksi paket berikutnya ~%ld s lagi -> sleep %lu ms",
              (long)sleepSec, (unsigned long)sleepMs);
    return sleepMs;
}
