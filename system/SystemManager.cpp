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
        // Sudah tahu jadwal transmisi -> cukup dengar sebentar di sekitar
        // waktu prediksi (guard window mengakomodasi jitter timing sensor).
        listenMs = TX_JITTER_GUARD_MS * 2;
        LOG_INFO("Mode sinkron (missed=%d): dengar %lu ms",
                 RTCMemory::getMissedCycles(), (unsigned long)listenMs);
    } else {
        // Belum sinkron / RTC tidak tersedia / terlalu banyak paket
        // hilang berturut-turut -> dengar penuh 1 periode transmisi
        // untuk menangkap paket pertama dan membangun ulang sinkronisasi.
        // Pakai cfg.listenWindow (bisa diatur dari server lewat GET
        // /config), bukan macro tetap, supaya config remote berpengaruh.
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
    if (count == 0) return;

    DeviceConfig cfg;
    ConfigManager config;
    config.begin();
    config.load(cfg);

    // SD dipakai sebagai penyimpanan PERSISTEN untuk data yang gagal
    // terkirim: beda dengan RTC memory yang hilang kalau ESP32 benar-benar
    // mati/reset total, file di SD tetap ada sampai data itu sukses masuk
    // ke database server.
    SDManager sd;
    bool sdReady = sd.begin() && sd.isPresent();
    status.sdCardPresent = sdReady;

    // Konek pakai kredensial yang sudah diketahui berhasil (tersimpan di NVS,
    // default-nya dari secrets.h saat pertama kali boot).
    WifiManager wifi;
    if (!wifi.connect(cfg.wifiSSID.c_str(), cfg.wifiPassword.c_str())) {
        LOG_ERROR("Gagal konek WiFi");
        if (sdReady) {
            // Pindahkan semua data yang baru terkumpul ke SD supaya tidak
            // hilang walau terjadi reset/mati total sebelum sempat konek
            // lagi ke WiFi.
            uint8_t moved = 0;
            for (uint8_t i = 0; i < count; i++) {
                WeatherData d;
                if (RTCMemory::getPending(i, d) && sd.queuePush(d)) moved++;
            }
            RTCMemory::clearPending();
            LOG_WARN("%d/%d data dipindah ke SD (gagal WiFi), buffer RTC dikosongkan", moved, count);
        } else {
            LOG_WARN("SD tidak tersedia, %d data tetap di RTC memory (berisiko hilang jika reset total)", count);
        }
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

    // ------------------------------------------------------------------
    // Sinkronisasi config dari server (GET /config), berbasis configVersion:
    //
    //  - configVersion==0 di lokal artinya BELUM PERNAH sinkron sama
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

            // Versi dicatat sebagai "sudah diproses" walau wifi gagal
            // switch, supaya tidak diulang-ulang tes tiap siklus di
            // versi yang sama. Kalau server memang mau device retry
            // terus, server perlu menaikkan configVersion lagi.
            cfg.configVersion = remote.configVersion;
            config.save(cfg);
            LOG_INFO("Config version sekarang: v%u (%s)",
                      (unsigned)cfg.configVersion,
                      changed ? "ada perubahan diterapkan" : "tidak ada field lain yang berubah");
        }
        // remote.configVersion == cfg.configVersion -> tidak ada perubahan, lewati diam-diam.
    }

    // 1) Kirim data yang baru terkumpul di siklus ini (RTC memory).
    uint8_t sent = 0;
    for (uint8_t i = 0; i < count; i++) {
        WeatherData d;
        if (!RTCMemory::getPending(i, d)) continue;
        if (api.uploadWeather(d)) {
            sent++;
        } else if (sdReady) {
            // Gagal terkirim -> simpan ke SD, dicoba lagi di siklus
            // upload berikutnya (sesuai interval kirim), bukan hilang.
            sd.queuePush(d);
        }
    }
    uint8_t unsent = count - sent;

    // 2) Coba kirim ulang antrian LAMA di SD (data dari siklus-siklus
    //    sebelumnya yang gagal terkirim). Yang sukses (masuk database)
    //    otomatis dihapus dari SD oleh queueFlush(); yang masih gagal
    //    tetap tersimpan untuk dicoba lagi di interval berikutnya.
    uint16_t sdSent = 0, sdRemaining = 0;
    if (sdReady) {
        sd.queueFlush(api, sdSent, sdRemaining);
    }

    status.wifiRSSI = (int8_t)wifi.getRSSI();
    api.uploadStatus(status);

    if (unsent == 0) {
        RTCMemory::clearPending();
        LOG_INFO("Upload sukses: %d/%d data terkirim langsung", sent, count);
    } else if (sdReady) {
        // Sisa yang gagal sudah aman tersalin ke SD di atas, RTC memory
        // boleh dikosongkan.
        RTCMemory::clearPending();
        LOG_WARN("Upload sebagian: %d/%d terkirim langsung, %d dipindah ke SD", sent, count, unsent);
    } else {
        // Tanpa SD, satu-satunya tempat data tersisa adalah RTC memory
        // (berisiko hilang jika terjadi reset total) -> jangan dikosongkan
        // supaya masih ada kesempatan retry di siklus berikutnya.
        LOG_WARN("Upload sebagian: %d/%d terkirim, SD tidak tersedia, %d tetap di RTC memory", sent, count, unsent);
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
