#include "headers.h"

WeatherData SystemManager::lastWeather;
DeviceStatus SystemManager::status;

uint32_t SystemManager::lastLightReadMs = 0;
uint32_t SystemManager::lastRadioWatchdogMs = 0;
uint32_t SystemManager::lastWifiCycleMs = 0;
bool SystemManager::firstWifiCycleDone = false;

// =====================================================================
// CATATAN ARSITEKTUR (MODE KONTINU)
// =====================================================================
// Beda dengan arsitektur lama (init() -> run() SEKALI -> deep sleep),
// di sini objek-objek periferal dibuat SEKALI dan HIDUP SEPANJANG
// RUNTIME (bukan variabel lokal yang dibuat ulang tiap "siklus").
// Radio, RTC, sensor cahaya, dan SD semuanya tetap aktif terus; hanya
// WiFi yang dinyala-matikan berkala (lihat doWifiCycle()).
// =====================================================================

static CC1101Driver  s_radio;
static WeatherDecoder s_decoder;
static LightSensor   s_light;
static RTCManager    s_rtc;
static SDManager     s_sd;
static PowerManager   s_power;

static bool s_rtcOk   = false;
static bool s_radioOk = false;
static bool s_lightOk = false;
static bool s_sdOk    = false;

// Buffer kerja untuk decode paket, dipakai ulang tiap panggilan supaya
// tidak alokasi stack besar berulang-ulang di tiap iterasi loop().
static uint32_t s_pulses[MAX_PULSES];
static uint8_t  s_bits[MAX_PULSES];

void SystemManager::init() {
    BootManager::init();
    Logger::setOutput(&Serial);
    Serial.begin(115200);
    delay(200);

    LOG_INFO("=== Boot #%lu (mode kontinu, tanpa deep sleep) ===",
             (unsigned long)PersistentState::getBootCount());

    Wire.begin(I2C_SDA, I2C_SCL);

    status.radioPresent = false;
    status.rtcPresent = false;
    status.lightSensorPresent = false;
    status.sdCardPresent = false;
    status.wifiCycleCount = 0;

    s_power.begin();

    s_rtcOk = s_rtc.begin() && s_rtc.isOK();
    status.rtcPresent = s_rtcOk;
    if (!s_rtcOk) {
        LOG_ERROR("RTC DS3231 tidak terdeteksi -- timestamp memakai fallback millis()/1000");
    }

    s_radioOk = s_radio.begin();
    status.radioPresent = s_radioOk;
    if (!s_radioOk) {
        LOG_ERROR("CC1101 gagal inisialisasi -- radio 433MHz TIDAK AKTIF pada boot ini");
    } else {
        LOG_INFO("Radio 433MHz aktif dalam mode continuous RX (tidak ada lagi jendela dengar)");
    }

    s_lightOk = s_light.begin();
    status.lightSensorPresent = s_lightOk;

    s_sdOk = s_sd.begin() && s_sd.isPresent();
    status.sdCardPresent = s_sdOk;
    if (!s_sdOk) {
        LOG_WARN("SD card tidak terdeteksi -- backup lokal & antrian offline tidak tersedia boot ini");
    }

    // Pulihkan akumulasi rain dari NVS SEKALI di sini (lihat catatan di
    // PersistentState/WeatherDecoder -- ini backup lintas brownout,
    // bukan lintas deep-sleep).
    s_decoder.loadRainState();

    // WiFi mati total sejak awal boot; baru dinyalakan oleh
    // runWifiCycleIfDue() saat jadwalnya tiba.
    WiFi.mode(WIFI_OFF);

    uint32_t now = millis();
    lastLightReadMs = now;
    lastRadioWatchdogMs = now;
    // lastWifiCycleMs sengaja "dimundurkan" penuh satu interval supaya
    // siklus WiFi PERTAMA langsung jalan begitu boot selesai (device
    // yang baru menyala perlu segera lapor status), bukan menunggu
    // WIFI_CYCLE_INTERVAL_MS penuh dulu.
    lastWifiCycleMs = now - WIFI_CYCLE_INTERVAL_MS;

    LOG_INFO("Inisialisasi selesai -- masuk mode operasi kontinu. "
             "WiFi akan menyala tiap ~%lu ms.", (unsigned long)WIFI_CYCLE_INTERVAL_MS);
}

void SystemManager::loop() {
    // Setiap fungsi di bawah ini non-blocking (berbasis millis()) KECUALI
    // doWifiCycle() (dipanggil dari runWifiCycleIfDue()), yang memang
    // sengaja memblokir sebentar selama proses konek+upload -- radio
    // 433MHz tetap menangkap pulsa lewat interrupt hardware selama itu,
    // jadi tidak ada data yang hilang, cuma diproses agak telat.
    pollRadio();
    pollLight();
    pollRadioWatchdog();
    runWifiCycleIfDue();
}

void SystemManager::pollLight() {
    if (!s_lightOk) return;
    uint32_t now = millis();
    if ((uint32_t)(now - lastLightReadMs) < LIGHT_READ_INTERVAL_MS) return;
    lastLightReadMs = now;
    s_light.readOnce(); // nilai disimpan internal, diambil lewat lastValue() saat paket radio datang
}

void SystemManager::pollRadioWatchdog() {
    if (!s_radioOk) return;
    uint32_t now = millis();
    if ((uint32_t)(now - lastRadioWatchdogMs) < RADIO_WATCHDOG_INTERVAL_MS) return;
    lastRadioWatchdogMs = now;
    s_radio.watchdogReset();
}

void SystemManager::pollRadio() {
    if (!s_radioOk) return;
    if (!s_radio.isPacketAvailable()) return;

    uint16_t count = s_radio.getPulseCount();
    s_radio.copyPulses(s_pulses, count);
    s_radio.resetPulseBuffer();

    uint16_t bitCount = s_decoder.pulsesToBits(s_pulses, count, s_bits, MAX_PULSES);
    if (bitCount < 64) {
        return; // kemungkinan noise / paket terlalu pendek -- radio otomatis lanjut RX
    }

    uint8_t packet[MAX_BYTES];
    uint8_t pktLen;
    if (!s_decoder.scanForPacket(s_bits, bitCount, packet, &pktLen)) {
        return; // tidak ketemu paket valid dalam pulsa ini -- lanjut dengar seperti biasa
    }

    // ---- Dedup: buang repeat dari transmisi yang sama ----
    // Sensor mengirim burst yang sama beberapa kali berturut-turut demi
    // keandalan (jarak antar-repeat cuma hitungan detik, jauh lebih
    // pendek dari jarak antar-transmisi asli ~48 detik). Kalau paket
    // valid ini datang terlalu dekat dengan paket valid sebelumnya,
    // anggap masih repeat yang sama dan jangan diproses lagi -- radio
    // tetap lanjut mendengarkan seperti biasa, cuma packet ini yang di-skip.
    uint32_t nowMs = millis();
    if (lastAcceptedPacketMs != 0 && (uint32_t)(nowMs - lastAcceptedPacketMs) < DUPLICATE_PACKET_WINDOW_MS) {
        LOG_INFO("Paket duplikat diabaikan (selisih %lu ms dari paket sebelumnya, ambang=%lu ms)",
                  (unsigned long)(nowMs - lastAcceptedPacketMs), (unsigned long)DUPLICATE_PACKET_WINDOW_MS);
        return;
    }

    uint32_t epoch = s_rtcOk ? s_rtc.now().unixtime() : (millis() / 1000);

    // Pasangkan dengan pembacaan cahaya PALING BARU. Di mode kontinu lux
    // sudah selalu di-update berkala oleh pollLight(), jadi di sini
    // tinggal dipakai (tidak perlu baca ulang sensor tiap paket datang).
    lastWeather.light = s_light.lastValue();

    float rainAcc;
    if (s_decoder.decodePacket(packet, pktLen, lastWeather, rainAcc, epoch)) {
        lastWeather.timestamp = epoch;
        lastAcceptedPacketMs = nowMs; // <-- BARU: tandai waktu paket valid ini untuk dedup paket berikutnya

        LOG_INFO("Paket diterima: T=%.1fC H=%d%% Wind=%.1fkm/h Rain=%.1fmm Lux=%.1f Batt=%s",
                  lastWeather.temperature, lastWeather.humidity,
                  lastWeather.windSpeed, lastWeather.rainTotal,
                  lastWeather.light, lastWeather.batteryOk ? "OK" : "LOW");

        s_decoder.saveRainState();

        if (!PersistentState::pushPending(lastWeather)) {
            LOG_WARN("Buffer pending penuh -- data ini akan hilang sebelum siklus WiFi berikutnya. "
                      "Pertimbangkan menaikkan PENDING_BUFFER_SIZE atau memperpendek WIFI_CYCLE_INTERVAL_MS.");
        }

        if (s_sdOk) {
            String line = String(lastWeather.timestamp) + ","
                        + String(lastWeather.temperature) + ","
                        + String(lastWeather.humidity) + ","
                        + String(lastWeather.windSpeed) + ","
                        + String(lastWeather.rainTotal) + ","
                        + String(lastWeather.light);
            s_sd.appendRecord(line.c_str());
        }
    }
}

void SystemManager::runWifiCycleIfDue() {
    DeviceConfig cfg;
    ConfigManager config;
    config.begin();
    config.load(cfg);

    uint32_t interval = cfg.wifiInterval;
    if (interval < MIN_WIFI_INTERVAL_MS) interval = MIN_WIFI_INTERVAL_MS;

    uint32_t now = millis();
    if ((uint32_t)(now - lastWifiCycleMs) < interval) return;

    lastWifiCycleMs = now;
    doWifiCycle();
}

void SystemManager::doWifiCycle() {
    status.wifiCycleCount++;
    LOG_INFO("=== Siklus WiFi #%lu dimulai ===", (unsigned long)status.wifiCycleCount);

    DeviceConfig cfg;
    ConfigManager config;
    config.begin();
    config.load(cfg);

    WifiManager wifi;
    if (!wifi.connect(cfg.wifiSSID.c_str(), cfg.wifiPassword.c_str())) {
        LOG_ERROR("Gagal konek WiFi, siklus ini dilewati (radio & sensor tetap jalan seperti biasa)");

        uint8_t count = PersistentState::pendingCount();
        if (s_sdOk && count > 0) {
            // Pindahkan data yang sudah terkumpul ke antrian SD supaya
            // tidak hilang kalau buffer pending penuh sebelum WiFi
            // berhasil konek lagi.
            bool moved_[PENDING_BUFFER_SIZE] = {false};
            uint8_t moved = 0;
            for (uint8_t i = 0; i < count && i < PENDING_BUFFER_SIZE; i++) {
                WeatherData d;
                bool ok = PersistentState::getPending(i, d) && s_sd.queuePush(d);
                moved_[i] = ok;
                if (ok) moved++;
            }
            bool keep[PENDING_BUFFER_SIZE];
            for (uint8_t i = 0; i < count && i < PENDING_BUFFER_SIZE; i++) keep[i] = !moved_[i];
            PersistentState::compactPending(keep, count);
            LOG_WARN("%d/%d data dipindah ke antrian SD (gagal WiFi), %d tetap di buffer (SD juga gagal)",
                      moved, count, count - moved);
        } else if (count > 0) {
            LOG_WARN("SD tidak tersedia & WiFi gagal -- %d data tetap di buffer, dicoba lagi siklus berikutnya", count);
        }

        wifi.disconnect();
        return;
    }

    // Konsumsi flag "butuh resync waktu" (di-set BootManager saat boot)
    // -- hanya terjadi sekali per boot, tepat saat WiFi pertama kali
    // berhasil konek.
    if (RTCManager::consumeNeedResyncFlag() && s_rtcOk) {
        s_rtc.syncFromNTP(NTP_SYNC_TIMEOUT_MS);
    }

    CloudAPI api;
    api.begin(cfg.serverURL.c_str(), cfg.apiKey.c_str());

    // ------------------------------------------------------------------
    // Sinkronisasi config dari server (GET /config), berbasis configVersion
    // -- logikanya sama persis dengan arsitektur lama, cuma field
    // listenWindow/sleepInterval/useDeepSleep sudah tidak relevan lagi
    // dan diganti wifiInterval.
    // ------------------------------------------------------------------
    RemoteConfig remote;
    if (api.fetchRemoteConfig(remote) && remote.hasConfigVersion) {
        if (cfg.configVersion == 0) {
            cfg.configVersion = remote.configVersion;
            config.save(cfg);
            LOG_INFO("Config version awal dicatat: v%u (konfigurasi lain belum diubah)",
                      (unsigned)cfg.configVersion);
        } else if (remote.configVersion != cfg.configVersion) {
            LOG_INFO("Config version server berubah (v%u -> v%u), sinkronisasi...",
                      (unsigned)cfg.configVersion, (unsigned)remote.configVersion);
            bool changed = false;

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

            if (remote.wifiInterval != 0 && remote.wifiInterval != cfg.wifiInterval) {
                cfg.wifiInterval = remote.wifiInterval;
                if (cfg.wifiInterval < MIN_WIFI_INTERVAL_MS) cfg.wifiInterval = MIN_WIFI_INTERVAL_MS;
                changed = true;
            }

            cfg.configVersion = remote.configVersion;
            config.save(cfg);
            LOG_INFO("Config version sekarang: v%u (%s)",
                      (unsigned)cfg.configVersion,
                      changed ? "ada perubahan diterapkan" : "tidak ada field lain yang berubah");
        }
        // configVersion sama -> tidak ada perubahan, lewati diam-diam.
    }

    // ---- 1) kirim data yang menumpuk di buffer sejak siklus WiFi terakhir ----
    uint8_t count = PersistentState::pendingCount();
    uint8_t sent = 0;
    bool keepInRtc[PENDING_BUFFER_SIZE] = {false};
    for (uint8_t i = 0; i < count && i < PENDING_BUFFER_SIZE; i++) {
        WeatherData d;
        if (!PersistentState::getPending(i, d)) continue;
        if (api.uploadWeather(d)) {
            sent++;
        } else if (s_sdOk && s_sd.queuePush(d)) {
            // sukses dipindah ke antrian SD -> aman dibuang dari buffer RAM
        } else {
            keepInRtc[i] = true;
        }
    }
    uint8_t unsent = count - sent;

    if (count > 0) {
        if (unsent == 0) {
            PersistentState::clearPending();
            LOG_INFO("Upload sukses: %d/%d data terkirim langsung", sent, count);
        } else {
            PersistentState::compactPending(keepInRtc, count);
            uint8_t stillLost = 0;
            for (uint8_t i = 0; i < count && i < PENDING_BUFFER_SIZE; i++) if (keepInRtc[i]) stillLost++;
            LOG_WARN("Upload sebagian: %d/%d terkirim langsung, %d dipindah ke SD, %d tetap di buffer (dicoba lagi)",
                      sent, count, (count - sent - stillLost), stillLost);
        }
    }

    // ---- 2) coba kirim ulang antrian lama di SD ----
    uint16_t sdSent = 0, sdRemaining = 0;
    if (s_sdOk) {
        static const uint16_t MAX_QUEUE_FLUSH_PER_CYCLE = 20;
        s_sd.queueFlush(api, sdSent, sdRemaining, MAX_QUEUE_FLUSH_PER_CYCLE);
        if (sdSent > 0 || sdRemaining > 0) {
            LOG_INFO("Antrian SD: %u data lama terkirim & dihapus, %u masih tertunda", sdSent, sdRemaining);
        }
    }

    // ---- 3) laporkan status device ----
    status.freeHeap = ESP.getFreeHeap();
    status.uptime = millis() / 1000;
    status.bootCount = PersistentState::getBootCount();
    strncpy(status.firmwareVersion, FW_VERSION, sizeof(status.firmwareVersion) - 1);
    status.firmwareVersion[sizeof(status.firmwareVersion) - 1] = '\0';
    status.batteryVoltage = s_power.readBatteryVoltage();
    status.superCapVoltage = s_power.readSuperCapVoltage();
    status.wifiRSSI = (int8_t)wifi.getRSSI();
    api.uploadStatus(status);

    wifi.disconnect();
    firstWifiCycleDone = true;
    LOG_INFO("=== Siklus WiFi #%lu selesai, WiFi dimatikan sampai ~%lu ms lagi ===",
             (unsigned long)status.wifiCycleCount, (unsigned long)cfg.wifiInterval);
}
