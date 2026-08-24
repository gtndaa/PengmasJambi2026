#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <FS.h>
#include <SD.h>

#include <SPI.h>
#include <BH1750.h>
#include <RTClib.h>
#include <SmartRC_CC1101.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <time.h>
#include "secrets.h"

// =====================================================================
//  1. IDENTITY
// =====================================================================
#define PROJECT_NAME        "WEATHER STATION ELED"
#define FW_VERSION          "1.1.0"
#define DEVICE_ID            "WS-ELED-001"

// =====================================================================
//  2. TIME CONFIG
// =====================================================================
#define UPLOAD_INTERVAL_MS  60000      // upload setiap 60 detik (dipakai sebagai target, dibulatkan ke siklus paket)
#define LISTEN_WINDOW_MS    48000      // durasi mendengar radio saat SINKRONISASI AWAL (belum tahu jadwal sensor)
#define SLEEP_INTERVAL_MS   0     // fallback deep sleep jika radio tidak pernah sinkron (5 menit)

#define TIMEZONE_OFFSET     0

// ---- KONFIGURASI DUTY-CYCLE BERBASIS ANALISIS PAKET (48 detik) -------
// Dari analisis log rtl_433-style: sensor cuaca (id 0x22 ch4) mengirim
// paket kira-kira setiap 48 detik, kadang meleset (miss 1-3 siklus).
// Strategi: setelah paket pertama berhasil didekode & timestamp RTC
// diketahui, ESP32 memprediksi waktu paket berikutnya dan hanya
// bangun sesaat sebelum itu (guard window), lalu deep sleep lagi.
#define TX_PERIOD_MS             48000UL   // interval nominal transmisi sensor
#define TX_JITTER_GUARD_MS        5000UL   // toleransi jitter DI SISI SETELAH prediksi
                                            // (dulu 4000; dinaikkan sedikit sbg
                                            // pengaman ekstra jitter oscillator TX asli)
#define WAKE_BEFORE_MS            4000UL   // bangun lebih awal dari prediksi.
                                            // Dulu 1500ms -- ternyata jauh kurang:
                                            // rtc.now() hanya presisi 1 detik penuh
                                            // (kuantisasi s/d 999ms), plus overhead
                                            // boot+init (delay(200) eksplisit + I2C/SPI
                                            // RTC/CC1101/BH1750) yang selama ini tidak
                                            // pernah dikurangkan dari margin ini. Net
                                            // effect: window dengar mulai jauh lebih
                                            // telat dari prediksi daripada yang
                                            // diasumsikan, sehingga TX yang datang pas
                                            // atau lebih awal dari jadwal keburu lewat
                                            // sebelum radio mulai listen.
#define MAX_MISSED_CYCLES         3       // setelah sekian kali miss beruntun -> re-sync (dengar penuh 1 periode)

// ---- NTP time sync (dipakai untuk koreksi drift RTC & recovery pasca-brownout) ----
// Server NTP publik + timeout tunggu respons. gmtOffset diset ke WIB
// (UTC+7) karena seluruh sistem sejauh ini konsisten memakai waktu
// lokal WIB (bukan UTC) -- cocokkan kalau device dipasang di zona lain.
#define NTP_SERVER_1              "pool.ntp.org"
#define NTP_SERVER_2              "time.google.com"
#define NTP_SERVER_3              "time.cloudflare.com"
#define NTP_GMT_OFFSET_SEC        (7 * 3600L)   // WIB = UTC+7
#define NTP_DAYLIGHT_OFFSET_SEC   0              // Indonesia tidak pakai DST
#define NTP_SYNC_TIMEOUT_MS       5000UL
#define MIN_SLEEP_MS              2000UL  // batas bawah supaya tidak sleep negatif/terlalu singkat
#define PENDING_BUFFER_SIZE        8      // jumlah pembacaan yang ditampung di RTC memory sebelum upload
#define UPLOAD_EVERY_N_CYCLES ((UPLOAD_INTERVAL_MS + TX_PERIOD_MS - 1) / TX_PERIOD_MS) // ~ setiap berapa siklus 48s baru upload

// =====================================================================
//  3. SERVER & API CONFIG
// =====================================================================
// URL ini sama dengan BASE_URL yang sudah diuji berhasil di esp_lambda_test.
#define SERVER_URL          "https://k27gamn56cmjkns7mcjny4wovu0jbems.lambda-url.ap-southeast-3.on.aws"
#define API_KEY             "your-api-key"

// Endpoint sesuai routes.js backend (JSON/routes/routes.js)
#define ENDPOINT_SENSOR_POST   "/sensordata"       // POST data cuaca (SensorData model)
#define ENDPOINT_SENSOR_GET    "/sensordata"       // GET data cuaca terbaru
#define ENDPOINT_CONFIG_GET    "/config"           // GET konfigurasi device terbaru (DeviceConfig model: wifi + interval dsb)

// =====================================================================
//  6. PINOUT CONFIGURATION
// =====================================================================

// CC1101
#define GDO2_PIN            4
#define GDO0_PIN            2
#define CC1101_SCK          18
#define CC1101_MISO         19
#define CC1101_MOSI         23
#define CC1101_CSN          5

// I2C (SDA/SCL)
#define I2C_SDA             21
#define I2C_SCL             22

// SD Card
#define SD_CS               33
#define SD_SCK              18
#define SD_MISO             19
#define SD_MOSI             23

// ADC supercapacitor / baterai
#define SUPERCAP_PIN        34
#define BATTERY_PIN         34   // tidak ada pembagi tegangan terpisah di board ini; gunakan pin yang sama

// =====================================================================
//  7. OTHER CONFIGS
// =====================================================================
#define RF_FREQ_MHZ         433.92f
#define RF_MODULATION       2           // ASK/OOK
#define RF_DATARATE         4.8f
#define RF_RXBW             203.12f

// Pulse timing untuk protokol WH5300 (dalam mikro detik)
#define PULSE_1_MIN         350
#define PULSE_1_MAX         650
#define PULSE_0_MIN         1100
#define PULSE_0_MAX         1590

#define MAX_PULSES          300
#define MAX_BYTES           12
#define PACKET_TIMEOUT      30          // ms tanpa pulsa baru -> commit paket

#define RAIN_MM_PER_TIP     0.3f
#define WIND_FACTOR         1.216f
#define TEMP_OFFSET         400
#define TEMP_DIVISOR        10.0f
#define WIND_DEG_STEP       22.5f

// PENTING: rain counter di paket radio ternyata 16-bit (byte tinggi di
// packet[6], byte rendah di packet[7]), BUKAN 8-bit di packet[6] saja
// seperti asumsi sebelumnya. Terverifikasi dari perbandingan delta
// counter vs pembacaan alat referensi bawaan -- rasio delta_counter /
// delta_mm_referensi persis 3.333 (=1/0.3) di 9 dari 9 titik data
// sample real, HANYA cocok kalau counter dibaca sebagai 16-bit
// gabungan packet[6]:packet[7], bukan packet[6] sendirian.
#define RAIN_UNINIT         0xFFFF

// Akumulator rain bertingkat (lihat RTCMemory::RTCData & WeatherDecoder)
#define RAIN_HOUR_BUCKET_COUNT      12      // 12 x 5 menit = rolling 60 menit
#define RAIN_HOUR_BUCKET_SPAN_SEC   300UL   // 5 menit per sub-bucket
#define SECONDS_PER_DAY             86400UL
#define SECONDS_PER_WEEK            604800UL
#define SECONDS_PER_MONTH           2592000UL // approksimasi 30 hari (kalender bulan asli bervariasi 28-31 hari)

#define CRC_POLY            0x31
#define CRC_INIT            0x00

// SD Card
#define SD_MAX_RECORDS      1000
#define SD_FILENAME         "/weather.log"
#define SD_QUEUE_FILENAME   "/queue.csv"   // antrian data yang belum berhasil dikirim ke server

// HTTP
#define HTTP_TIMEOUT        10000
#define MAX_RETRIES         3

// Logging level (ubah untuk debug)
#ifndef LOG_LEVEL
#define LOG_LEVEL           LOG_LEVEL_INFO
#endif
// =====================================================================
//  4. DATA STRUCTURES
// =====================================================================

// 4.1 WeatherData
struct WeatherData {
    uint32_t timestamp;          // Unix epoch
    float    temperature;        // C
    uint8_t  humidity;           // %
    float    windSpeed;          // km/h
    float    windGust;           // km/h
    uint8_t  windDirection;      // indeks 0-15
    float    windDeg;            // derajat
    float    rainDelta;          // mm sejak terakhir
    float    rainTotal;          // mm akumulasi
    uint16_t rainRaw;            // counter mentah, 16-bit (packet[6]:packet[7])
    float    light;              // lux
    uint8_t  sensorId;
    uint8_t  channel;
    bool     batteryOk;
};

// 4.2 DeviceStatus
struct DeviceStatus {
    float  batteryVoltage;       // Volt
    float  superCapVoltage;      // Volt
    int8_t wifiRSSI;             // dBm
    uint32_t freeHeap;
    uint32_t uptime;             // detik
    uint32_t bootCount;
    uint32_t wakeCounter;
    char   firmwareVersion[16];
    bool   sdCardPresent;
    bool   rtcPresent;
    bool   lightSensorPresent;
    bool   radioPresent;
};

// 4.3 DeviceConfig
struct DeviceConfig {
    String wifiSSID;
    String wifiPassword;
    String serverURL;
    String apiKey;
    unsigned long uploadInterval = UPLOAD_INTERVAL_MS;   // ms
    unsigned long listenWindow = LISTEN_WINDOW_MS;        // ms
    unsigned long sleepInterval = SLEEP_INTERVAL_MS;      // ms
    bool useDeepSleep = true;

    // Versi konfigurasi terakhir yang sudah diproses dari server.
    // 0 = belum pernah sinkron sama sekali (kondisi awal / first boot).
    uint32_t configVersion = 0;
};

// 4.4 WifiCredentials
// Struct minimal, hanya ssid + password. Dipakai oleh
// WifiManager::connectWithFallback() untuk tes-konek SSID baru.
struct WifiCredentials {
    String ssid;
    String password;
};

// 4.5 RemoteConfig
// Bentuk data yang diterima dari endpoint GET /config (backend
// DeviceConfig model). ssid/password diperlakukan khusus (dicoba
// konek dulu sebelum dipakai permanen), field lain diterapkan
// langsung kalau nilainya beda dari config yang tersimpan.
struct RemoteConfig {
    String   ssid;
    String   password;
    uint32_t uploadInterval = 0;   // 0 = tidak dikirim server / abaikan
    uint32_t listenWindow = 0;
    uint32_t sleepInterval = 0;
    uint32_t configVersion = 0;
    bool     useDeepSleep = true;
    bool     hasConfigVersion = false; // true kalau field ini ada di response
};

// =====================================================================
//  5. CLASS DECLARATIONS
// =====================================================================

// --------------------------- 5.1 Logger -------------------------------
#define LOG_LEVEL_DEBUG   0
#define LOG_LEVEL_INFO    1
#define LOG_LEVEL_WARN    2
#define LOG_LEVEL_ERROR   3

#define LOG_INFO(fmt, ...)   Logger::log(LOG_LEVEL_INFO,   "[INFO] " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)   Logger::log(LOG_LEVEL_WARN,   "[WARN] " fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)  Logger::log(LOG_LEVEL_ERROR,  "[ERROR] " fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)  Logger::log(LOG_LEVEL_DEBUG,  "[DEBUG] " fmt, ##__VA_ARGS__)

class Logger {
public:
    static void log(int level, const char* format, ...);
    static void setOutput(Print* output = &Serial);
private:
    static Print* out;
};

// --------------------------- 5.2 RTCMemory ----------------------------
// Menyimpan data di RTC memory ESP32 (tidak hilang saat deep sleep)
class RTCMemory {
public:
    static void init();          // panggil di setup
    static uint32_t getBootCount();
    static uint32_t getWakeCounter();
    static void incrementBootCount();
    static void incrementWakeCounter();
    static void setConfigVersion(uint8_t ver);
    static uint8_t getConfigVersion();
    static void saveBuffer(uint8_t* data, size_t len);
    static bool loadBuffer(uint8_t* data, size_t maxLen, size_t& outLen);

    // ---- state sinkronisasi jadwal paket radio (untuk duty-cycle 48s) ----
    static uint32_t getLastPacketEpoch();
    static void setLastPacketEpoch(uint32_t epoch);
    static bool isSynced();
    static uint8_t getMissedCycles();
    static void incrementMissedCycles();
    static void resetMissedCycles();

    // ---- buffer pending upload (bertahan lintas deep-sleep) ----
    static bool pushPending(const WeatherData& data);
    static uint8_t pendingCount();
    static bool getPending(uint8_t index, WeatherData& out);
    static void clearPending();
    static void compactPending(const bool keep[], uint8_t count);

    // ---- akumulasi rain, tetap konsisten walau deep sleep ----
    static uint16_t getRainCounterPrev();
    static void setRainCounterPrev(uint16_t v);
    static float getRainAccumulated();
    static void setRainAccumulated(float v);
    static void persistRainStateToNVS();

    // ---- akumulator bertingkat ----
    static float getRainHourBucket(uint8_t i);
    static uint32_t getRainHourBucketSlot(uint8_t i);
    static void setRainHourBucket(uint8_t i, float mm, uint32_t slot);

    static float getRain24h();
    static uint32_t getRain24hStartEpoch();
    static void setRain24h(float mm, uint32_t startEpoch);

    static float getRainWeek();
    static uint32_t getRainWeekStartEpoch();
    static void setRainWeek(float mm, uint32_t startEpoch);

    static float getRainMonth();
    static uint32_t getRainMonthStartEpoch();
    static void setRainMonth(float mm, uint32_t startEpoch);

    // Struct internal harus public agar bisa dideklarasikan sebagai
    // RTC_DATA_ATTR static RTCMemory::RTCData di file .cpp (di luar kelas).
    struct RTCData {
        uint32_t bootCount;
        uint32_t wakeCounter;
        uint8_t  configVersion;
        uint8_t  buffer[256];
        uint16_t bufferLen;

        // penjadwalan radio
        uint32_t lastPacketEpoch;      // epoch RTC saat paket valid terakhir diterima (0 = belum sinkron)
        uint8_t  missedCycles;         // berapa kali berturut-turut window dengar tidak menangkap paket

        // buffer upload
        WeatherData pending[PENDING_BUFFER_SIZE];
        uint8_t     pendingLen;

        // decoder rain state (harus persist lintas deep-sleep)
        uint16_t rainCounterPrev;
        float   rainAccumulated;

        // ---- akumulator rain bertingkat (1h rolling + 24h/week/month kalender) ----
        // 1 jam terakhir: rolling window sungguhan, dipecah jadi 12 sub-bucket
        // 5 menitan (bukan reset kalender di jam:00) supaya "1 jam terakhir"
        // selalu representasi 60 menit paling baru, bukan sisa jam berjalan.
        float    rainHourBucket[RAIN_HOUR_BUCKET_COUNT];
        uint32_t rainHourBucketSlot[RAIN_HOUR_BUCKET_COUNT]; // epoch/RAIN_HOUR_BUCKET_SPAN_SEC saat bucket ini terakhir dipakai

        // 24h/week/month: bucket KALENDER (reset saat masuk hari/minggu/bulan
        // baru), mengikuti pola alat referensi bawaan. Disimpan juga ke NVS
        // (lihat persistRainStateToNVS) supaya tidak hilang saat cold-boot.
        float    rain24h;
        uint32_t rain24hStartEpoch;
        float    rainWeek;
        uint32_t rainWeekStartEpoch;
        float    rainMonth;
        uint32_t rainMonthStartEpoch;
    };

private:
    static RTCData* rtcData;
};

// --------------------------- 5.3 RTCManager ---------------------------
class RTCManager {
public:
    bool begin();
    bool isOK() const;
    DateTime now();
    float getTemperature();
    String dateTimeStr();
    String isoStr();
    void adjust(const DateTime& dt);

    // Sync waktu dari server NTP asli (bukan header HTTP) via internet
    // biasa (UDP/123), TIDAK menyalakan WiFi sendiri -- dipanggil hanya
    // saat WiFi memang sudah konek (numpang sesi upload yang sudah ada).
    // Return false kalau gagal (mis. jaringan blokir UDP/123, timeout) --
    // RTC dibiarkan apa adanya, TIDAK di-fallback ke compile time.
    bool syncFromNTP(uint32_t timeoutMs = 5000);

    // Flag "butuh re-sync waktu", disimpan di NVS (bukan RTC memory,
    // supaya tetap ada walau brownout ikut menghapus RTC memory).
    // Di-set oleh BootManager saat mendeteksi reset sebab brownout/
    // power-on, dikonsumsi (dibaca + dihapus) oleh SystemManager saat
    // WiFi tersedia untuk mencoba NTP sync.
    static void markNeedResync();
    static bool consumeNeedResyncFlag(); // true jika ada flag & langsung dihapus dari NVS
private:
    RTC_DS3231 rtc;
    bool ok = false;
};

// --------------------------- 5.4 SDManager ----------------------------
class CloudAPI; // forward declare, definisi lengkap ada di bawah (5.6)

class SDManager {
public:
    bool begin();
    bool isPresent() const;
    bool appendRecord(const char* data);      // tambahkan baris ke file log biasa

    // ---- Antrian data yang gagal terkirim, disimpan persisten di SD ----
    // (beda dengan RTC memory: RTC memory hilang kalau ESP32 benar-benar
    // mati/reset total, sedangkan file di SD tetap ada).
    bool queuePush(const WeatherData& d);     // simpan 1 data yang gagal terkirim
    uint16_t queueCount();                    // jumlah data yang masih tertunda di SD

    // Coba kirim ulang semua data di antrian lewat `api`. Data yang
    // berhasil terkirim (server balas sukses / masuk database) langsung
    // dihapus dari file SD; yang masih gagal tetap disimpan untuk
    // dicoba lagi di siklus upload berikutnya.
    // outSent  = jumlah yang berhasil terkirim & dihapus dari SD
    // outRemaining = jumlah yang masih tersisa di SD setelah percobaan ini
    // maxRecords=0 berarti tanpa batas. Sengaja diberi default terbatas
    // di pemanggil (lihat SystemManager::upload()) supaya satu siklus
    // upload tidak memblokir radio terlalu lama kalau backlog besar.
    void queueFlush(CloudAPI& api, uint16_t& outSent, uint16_t& outRemaining,
                     uint16_t maxRecords = 0);

private:
    bool present = false;
    File file;
    String filename = SD_FILENAME;
    String queueFilename = SD_QUEUE_FILENAME;

    static String encodeRecord(const WeatherData& d);
    static bool decodeRecord(const String& line, WeatherData& out);
};

// --------------------------- 5.5 LightSensor --------------------------
class LightSensor {
public:
    bool begin();
    bool isReady() const;   // true jika begin() terakhir berhasil mendeteksi sensor
    float readOnce();       // baca sekali langsung (dipakai saat wake singkat)
    void powerDown();       // matikan sensor sebelum deep sleep
private:
    BH1750 sensor;
    float lux = 0.0f;
    uint32_t readySince = 0;
    bool ready = false;
};

// --------------------------- 5.6 CC1101Driver -------------------------
class CC1101Driver {
public:
    bool begin();
    void setReceiveMode();
    bool isPacketAvailable();          // periksa GDO2/ interrupt
    void resetPulseBuffer();
    uint16_t getPulseCount();
    void copyPulses(uint32_t* dest, uint16_t maxCount);
    void enableInterrupt();
    void disableInterrupt();
    void watchdogReset();              // panggil SetRx() secara periodik
    void sleepRadio();                 // idle-kan CC1101 sebelum deep sleep (hemat daya)

    // Re-assert HANYA pin routing SPI milik CC1101 (tanpa Init()/SetRx()/
    // attachInterrupt seperti begin() penuh) -- dipakai tepat sebelum
    // sleepRadio() untuk memastikan command goSleep() terkirim ke pin
    // yang benar walau SD sempat mengubah config SPI global di antaranya.
    // SENGAJA TIDAK memanggil begin() penuh lagi di sini: begin()+
    // sleepRadio() beruntun tanpa jeda pernah dicoba dan malah membuat
    // CC1101 gagal terdeteksi di siklus berikutnya (kemungkinan
    // melanggar minimum settling time CC1101 antar transisi mode
    // reset->RX->sleep yang terlalu rapat). Re-assert pin saja jauh
    // lebih ringan -- cuma update variabel routing di library, tidak
    // mengirim command SPI/reset apapun ke chip.
    void reassertSpiPins();
};

// --------------------------- 5.7 WeatherDecoder ----------------------
class WeatherDecoder {
public:
    // Mengubah array pulsa menjadi bit (0/1)
    uint16_t pulsesToBits(uint32_t* pulses, uint16_t count, uint8_t* bits, uint16_t maxBits);

    // Mencari paket valid di dalam aliran bit
    bool scanForPacket(uint8_t* bits, uint16_t bitCount, uint8_t* out, uint8_t* outLen);

    // Mendekode paket menjadi WeatherData (termasuk rain delta dan akumulasi).
    // `epoch` = waktu RTC saat ini (dipakai untuk bucket rolling 1h &
    // kalender 24h/week/month) -- diambil oleh caller SEBELUM memanggil
    // fungsi ini.
    bool decodePacket(uint8_t* packet, uint8_t len, WeatherData& data, float& rainAccumulated, uint32_t epoch);

    // Hitung CRC-8 (polynomial 0x31)
    uint8_t crc8(uint8_t* data, uint8_t len);

    // Reset state rain counter (dipanggil saat boot)
    void resetRainCounter(uint16_t initialCounter = RAIN_UNINIT);

    // Muat/simpan state rain counter dari/ke RTC memory (persist lintas deep-sleep)
    void loadRainStateFromRTC();
    void saveRainStateToRTC();

private:
    uint16_t rainCounterPrev = RAIN_UNINIT;
    float rainAccumulated = 0.0f;

    // ---- akumulator bertingkat (1h rolling + 24h/week/month kalender) ----
    float    hourBucket[RAIN_HOUR_BUCKET_COUNT] = {0};
    uint32_t hourBucketSlot[RAIN_HOUR_BUCKET_COUNT] = {0};
    float    rain24h = 0.0f;
    uint32_t rain24hStart = 0;
    float    rainWeek = 0.0f;
    uint32_t rainWeekStart = 0;
    float    rainMonth = 0.0f;
    uint32_t rainMonthStart = 0;

    // Tambahkan `deltaMm` ke semua tingkat akumulator (dipanggil setiap
    // ada delta rain baru, sebelum dikembalikan sebagai rain_delta).
    // `epoch` dipakai untuk menentukan sub-bucket 1h mana yang kena, dan
    // untuk mendeteksi kapan bucket kalender 24h/week/month harus reset.
    void addToTieredAccumulators(float deltaMm, uint32_t epoch);

    // Jumlahkan 12 sub-bucket 1 jam terakhir, sambil membuang (nolkan)
    // sub-bucket yang sudah lewat >60 menit dari `epoch` saat ini.
    float sumRollingHour(uint32_t epoch);
};

// --------------------------- 5.8 PowerManager -------------------------
class PowerManager {
public:
    void begin();
    float readBatteryVoltage() const;      // volt
    float readSuperCapVoltage() const;     // volt
    void prepareDeepSleep(uint64_t wakeUpTimeUs);
    void deepSleepNow();
};

// --------------------------- 5.9 CloudAPI -----------------------------
class CloudAPI {
public:
    CloudAPI();

    bool begin(const char* serverURL, const char* apiKey);

    bool uploadWeather(const WeatherData& data);
    bool uploadStatus(const DeviceStatus& status);

    // Ambil konfigurasi terbaru dari endpoint GET /config (backend
    // DeviceConfig model): wifiSSID, wifiPassword, uploadInterval,
    // listenWindow, sleepInterval, configVersion, useDeepSleep.
    bool fetchRemoteConfig(RemoteConfig& out);

private:
    String server;
    String apiKey;
};

// --------------------------- 5.10 ConfigManager ------------------------
class ConfigManager {
public:
    ConfigManager();
    bool begin(const char* namespaceName = "wscfg");
    bool load(DeviceConfig& cfg);
    bool save(const DeviceConfig& cfg);
    bool updateFromJSON(const String& json);
private:
    Preferences prefs;
    String ns = "wscfg";
};

// --------------------------- 5.11 WifiManager --------------------------
class WifiManager {
public:
    WifiManager();
    bool connect(const char* ssid, const char* password, unsigned long timeoutMs = 15000);

    // Coba ganti koneksi ke kredensial baru (mis. dari server). Jika
    // gagal terhubung sampai timeout, otomatis kembali (fallback) ke
    // ssid/password lama yang sudah terbukti berhasil.
    // Return true jika berhasil pindah ke kredensial BARU.
    // Return false jika gagal & sudah kembali memakai kredensial lama.
    bool connectWithFallback(const WifiCredentials& newCreds,
                              const char* fallbackSsid,
                              const char* fallbackPassword,
                              unsigned long timeoutMs = 15000);

    int getRSSI() const;
    void disconnect();
};

// --------------------------- 5.12 Helpers ----------------------------
class Helpers {
public:
    // Format "YYYY-MM-DD HH:MM:SS" dari epoch, sesuai format field
    // "datetime" pada backend SensorData (lihat esp_lambda_test payload).
    static String epochToDateTimeStr(uint32_t epoch);

    // Konversi index arah angin (0-15) ke string kompas (N, NNE, NE, ...),
    // sesuai field "wind_dir" (String) pada backend SensorData.
    static const char* windDirToStr(uint8_t dirIndex);
};

// --------------------------- 5.13 SystemManager ----------------------
// Catatan arsitektur: ESP32 deep sleep = reset penuh (RAM hilang, hanya
// RTC memory yang bertahan). Karena itu SystemManager TIDAK berjalan
// sebagai loop() terus-menerus; SystemManager::run() dieksekusi SEKALI
// per siklus bangun (dipanggil dari setup()), lalu MCU deep sleep lagi
// via Scheduler::goToSleep(). Semua state lintas-siklus (jadwal radio,
// buffer upload, akumulasi hujan) disimpan di RTCMemory.
class SystemManager {
public:
    static void init();
    static void run();              // satu siklus kerja: dengar radio -> baca lux -> (mungkin) upload -> hitung waktu sleep berikutnya
    static void receiveWeather(uint32_t listenWindowMs);
    static void readLight(LightSensor& light);
    static void storeData();
    static void upload();
    static uint32_t computeNextSleepMs(); // dihitung setelah run(), dipakai Scheduler::goToSleep()
private:
    static WeatherData lastWeather;
    static DeviceStatus status;
    static bool packetReceivedThisCycle;
    // Prediksi epoch transmisi berikutnya dari referensi paket valid
    // terakhir. Dipakai baik untuk menentukan listenMs (run()) maupun
    // durasi sleep (computeNextSleepMs()) -- satu sumber kebenaran,
    // supaya keduanya selalu konsisten relatif terhadap prediksi yang sama.
    static uint32_t predictNextExpectedEpoch(uint32_t lastPkt, uint32_t now);
};

// --------------------------- 5.14 Scheduler --------------------------
class Scheduler {
public:
    static void goToSleep();                 // pakai durasi hasil SystemManager::computeNextSleepMs()
    static void goToSleep(uint32_t sleepMs);  // durasi eksplisit
};

// --------------------------- 5.15 BootManager ------------------------
class BootManager {
public:
    static void init();
    static uint32_t getBootCount();
};

