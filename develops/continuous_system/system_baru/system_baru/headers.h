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
#include <time.h>
#include "secrets.h"

// =====================================================================
//  1. IDENTITY
// =====================================================================
#define PROJECT_NAME        "WEATHER STATION ELED (CONTINUOUS)"
#define FW_VERSION          "2.0.0"
#define DEVICE_ID            "WS-ELED-001"

// =====================================================================
//  2. TIME CONFIG (mode kontinu -- TIDAK ADA DEEP SLEEP)
// =====================================================================
// Arsitektur lama membangunkan ESP32 dari deep sleep sesaat sebelum
// jadwal paket radio berikutnya (duty-cycle 48 detik). Di mode kontinu
// ini SEMUA fungsi (radio, sensor cahaya, SD) berjalan terus tanpa
// henti di loop() -- jadi tidak perlu lagi memprediksi jadwal paket
// atau menghitung durasi "dengar radio". Satu-satunya bagian yang
// tetap di-duty-cycle adalah WIFI, karena itu bagian paling boros daya
// dan tidak perlu selalu menyala untuk sekadar menerima data 433MHz.

#define TIMEZONE_OFFSET     0

// ---- DUTY-CYCLE WIFI (satu-satunya periferal yang di-duty-cycle) -----
// WiFi dinyalakan berkala setiap WIFI_CYCLE_INTERVAL_MS, dipakai untuk:
// upload data yang menumpuk di buffer, flush antrian SD, sinkronisasi
// config dari server, lalu dimatikan lagi. Di luar jendela ini radio
// 433MHz, sensor cahaya, dan pencatatan SD tetap berjalan seperti
// biasa (TIDAK ikut mati/duty-cycle).
#define WIFI_CYCLE_INTERVAL_MS     (5UL * 60UL * 1000UL)   // nyalakan WiFi tiap 5 menit
#define WIFI_CONNECT_TIMEOUT_MS    15000UL                  // batas tunggu koneksi per percobaan

// ---- Pembacaan sensor cahaya kontinu ----
// Dulu lux hanya dibaca sekali per siklus bangun (readOnce() dipanggil
// tepat sebelum sleep lagi). Sekarang dibaca berkala sepanjang waktu
// supaya nilainya selalu representasi kondisi PALING BARU saat paket
// radio berhasil didekode.
#define LIGHT_READ_INTERVAL_MS     2000UL   // baca BH1750 tiap 2 detik

// ---- Watchdog radio ----
// Sesekali panggil ulang SetRx() sebagai jaga-jaga kalau CC1101
// "nyangkut" (mis. gara-gara noise RF terus-menerus) -- di arsitektur
// lama ini dipanggil tiap wake, sekarang dipanggil berkala di loop().
#define RADIO_WATCHDOG_INTERVAL_MS 10000UL

#define MIN_WIFI_INTERVAL_MS       10000UL  // batas bawah interval WiFi dari server, hindari 0 -> spam konek

#define PENDING_BUFFER_SIZE        16       // jumlah pembacaan yang ditampung sebelum WiFi berikutnya menyala
                                             // (5 menit / ~48s per paket = ~6 paket, diberi margin)

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

// =====================================================================
//  3. SERVER & API CONFIG
// =====================================================================
#define SERVER_URL          "https://k27gamn56cmjkns7mcjny4wovu0jbems.lambda-url.ap-southeast-3.on.aws"
#define API_KEY             "your-api-key"

// Endpoint sesuai routes.js backend (JSON/routes/routes.js)
#define ENDPOINT_SENSOR_POST   "/sensordata"       // POST data cuaca (SensorData model)
#define ENDPOINT_SENSOR_GET    "/sensordata"       // GET data cuaca terbaru
#define ENDPOINT_CONFIG_GET    "/config"           // GET konfigurasi device terbaru (DeviceConfig model)

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

// Rain counter di paket radio adalah 16-bit (byte tinggi di packet[6],
// byte rendah di packet[7]). Terverifikasi dari perbandingan delta
// counter vs pembacaan alat referensi bawaan.
#define RAIN_UNINIT         0xFFFF

// Akumulator rain bertingkat (lihat PersistentState::RainState & WeatherDecoder)
#define RAIN_HOUR_BUCKET_COUNT      12      // 12 x 5 menit = rolling 60 menit
#define RAIN_HOUR_BUCKET_SPAN_SEC   300UL   // 5 menit per sub-bucket
#define SECONDS_PER_DAY             86400UL
#define SECONDS_PER_WEEK            604800UL
#define SECONDS_PER_MONTH           2592000UL // approksimasi 30 hari

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
    uint32_t uptime;             // detik sejak boot (millis()/1000, mode kontinu = tidak pernah reset kecuali reboot sungguhan)
    uint32_t bootCount;
    uint32_t wifiCycleCount;     // berapa kali WiFi sudah dinyalakan sejak boot
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
    unsigned long wifiInterval = WIFI_CYCLE_INTERVAL_MS;  // ms, seberapa sering WiFi dinyalakan

    // Versi konfigurasi terakhir yang sudah diproses dari server.
    // 0 = belum pernah sinkron sama sekali (kondisi awal / first boot).
    uint32_t configVersion = 0;
};

// 4.4 WifiCredentials
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
    uint32_t wifiInterval = 0;     // 0 = tidak dikirim server / abaikan
    uint32_t configVersion = 0;
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

// --------------------------- 5.2 PersistentState -----------------------
// Pengganti RTCMemory dari arsitektur lama. Karena TIDAK ADA deep sleep
// lagi, tidak ada gunanya menyimpan state di RTC_DATA_ATTR (fiturnya
// "bertahan lintas deep sleep" -- kalau tidak pernah deep sleep, itu
// sama saja dengan variabel biasa di RAM). Jadi seluruh state sesi
// (buffer pending upload, dsb.) sekarang cukup disimpan sebagai
// variabel statis biasa.
//
// PENGECUALIAN: akumulasi rain (rainAccumulated, rain24h/week/month,
// dst.) TETAP di-backup ke NVS (flash), sama seperti arsitektur lama --
// bukan karena deep sleep, tapi supaya tidak hilang kalau device
// sempat brownout/mati listrik sesaat lalu reboot (device ini beberapa
// kali mengalami POWERON_RESET akibat tegangan kurang stabil).
class PersistentState {
public:
    static void init();          // panggil sekali di setup()

    static uint32_t getBootCount();

    // ---- buffer pending upload (menumpuk sampai WiFi berikutnya menyala) ----
    static bool pushPending(const WeatherData& data);
    static uint8_t pendingCount();
    static bool getPending(uint8_t index, WeatherData& out);
    static void clearPending();
    static void compactPending(const bool keep[], uint8_t count);

    // ---- akumulasi rain, persisten lintas brownout/reboot (via NVS) ----
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

private:
    static uint32_t bootCount;

    static WeatherData pending[PENDING_BUFFER_SIZE];
    static uint8_t pendingLen;

    static uint16_t rainCounterPrev;
    static float rainAccumulated;
    static float rainHourBucket[RAIN_HOUR_BUCKET_COUNT];
    static uint32_t rainHourBucketSlot[RAIN_HOUR_BUCKET_COUNT];
    static float rain24h;
    static uint32_t rain24hStartEpoch;
    static float rainWeek;
    static uint32_t rainWeekStartEpoch;
    static float rainMonth;
    static uint32_t rainMonthStartEpoch;
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

    // Sync waktu dari server NTP asli via internet biasa (UDP/123),
    // TIDAK menyalakan WiFi sendiri -- dipanggil hanya saat WiFi
    // memang sudah konek (numpang sesi WiFi kontinu yang sudah ada).
    bool syncFromNTP(uint32_t timeoutMs = 5000);

    // Flag "butuh re-sync waktu", disimpan di NVS. Di-set saat boot
    // mendeteksi reset sebab brownout/power-on, dikonsumsi (dibaca +
    // dihapus) saat WiFi pertama kali tersedia untuk mencoba NTP sync.
    static void markNeedResync();
    static bool consumeNeedResyncFlag();
private:
    RTC_DS3231 rtc;
    bool ok = false;
};

// --------------------------- 5.4 SDManager ----------------------------
class CloudAPI; // forward declare

class SDManager {
public:
    bool begin();
    bool isPresent() const;
    bool appendRecord(const char* data);      // tambahkan baris ke file log biasa

    // ---- Antrian data yang gagal terkirim, disimpan persisten di SD ----
    bool queuePush(const WeatherData& d);
    uint16_t queueCount();
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
    bool isReady() const;
    float readOnce();       // baca sekali (dipanggil berkala di loop kontinu)
    float lastValue() const { return lux; }
private:
    BH1750 sensor;
    float lux = 0.0f;
    uint32_t readySince = 0;
    bool ready = false;
};

// --------------------------- 5.6 CC1101Driver -------------------------
// Radio SELALU dalam mode terima (continuous RX) -- tidak ada lagi
// konsep "jendela dengar" karena tidak ada deep sleep untuk dihemat.
class CC1101Driver {
public:
    bool begin();               // init + langsung masuk continuous RX
    void setReceiveMode();
    bool isPacketAvailable();
    void resetPulseBuffer();
    uint16_t getPulseCount();
    void copyPulses(uint32_t* dest, uint16_t maxCount);
    void watchdogReset();       // panggil SetRx() secara periodik (anti-nyangkut)
};

// --------------------------- 5.7 WeatherDecoder ----------------------
class WeatherDecoder {
public:
    uint16_t pulsesToBits(uint32_t* pulses, uint16_t count, uint8_t* bits, uint16_t maxBits);
    bool scanForPacket(uint8_t* bits, uint16_t bitCount, uint8_t* out, uint8_t* outLen);

    // `epoch` = waktu RTC saat ini (dipakai untuk bucket rolling 1h &
    // kalender 24h/week/month) -- diambil oleh caller SEBELUM memanggil
    // fungsi ini.
    bool decodePacket(uint8_t* packet, uint8_t len, WeatherData& data, float& rainAccumulated, uint32_t epoch);

    uint8_t crc8(uint8_t* data, uint8_t len);

    void resetRainCounter(uint16_t initialCounter = RAIN_UNINIT);

    // Muat sekali di boot / simpan tiap ada delta baru (lihat catatan
    // arsitektur di PersistentState -- ini backup lintas brownout,
    // BUKAN backup lintas deep-sleep karena deep sleep sudah tidak ada).
    void loadRainState();
    void saveRainState();

private:
    uint16_t rainCounterPrev = RAIN_UNINIT;
    float rainAccumulated = 0.0f;

    float    hourBucket[RAIN_HOUR_BUCKET_COUNT] = {0};
    uint32_t hourBucketSlot[RAIN_HOUR_BUCKET_COUNT] = {0};
    float    rain24h = 0.0f;
    uint32_t rain24hStart = 0;
    float    rainWeek = 0.0f;
    uint32_t rainWeekStart = 0;
    float    rainMonth = 0.0f;
    uint32_t rainMonthStart = 0;

    void addToTieredAccumulators(float deltaMm, uint32_t epoch);
    float sumRollingHour(uint32_t epoch);
};

// --------------------------- 5.8 PowerManager -------------------------
// Tanpa deep sleep, PowerManager cukup urus pembacaan tegangan saja.
class PowerManager {
public:
    void begin();
    float readBatteryVoltage() const;
    float readSuperCapVoltage() const;
};

// --------------------------- 5.9 CloudAPI -----------------------------
class CloudAPI {
public:
    CloudAPI();

    bool begin(const char* serverURL, const char* apiKey);

    bool uploadWeather(const WeatherData& data);
    bool uploadStatus(const DeviceStatus& status);

    // Ambil konfigurasi terbaru dari endpoint GET /config: wifiSSID,
    // wifiPassword, wifiInterval, configVersion.
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
    bool connect(const char* ssid, const char* password, unsigned long timeoutMs = WIFI_CONNECT_TIMEOUT_MS);

    bool connectWithFallback(const WifiCredentials& newCreds,
                              const char* fallbackSsid,
                              const char* fallbackPassword,
                              unsigned long timeoutMs = WIFI_CONNECT_TIMEOUT_MS);

    int getRSSI() const;
    void disconnect();          // disconnect() + WiFi.mode(WIFI_OFF), radio WiFi benar2 mati sampai siklus berikutnya
};

// --------------------------- 5.12 Helpers ----------------------------
class Helpers {
public:
    static String epochToDateTimeStr(uint32_t epoch);
    static const char* windDirToStr(uint8_t dirIndex);
};

// --------------------------- 5.13 SystemManager ----------------------
// Catatan arsitektur (MODE KONTINU -- TIDAK ADA DEEP SLEEP):
// SystemManager::init() dipanggil SEKALI di setup(). Setelah itu
// SystemManager::loop() dipanggil BERULANG-ULANG dari loop() Arduino,
// dan menjalankan semua pekerjaan secara non-blocking (berbasis
// millis()) di setiap iterasi:
//   - radio 433MHz  : selalu dalam mode terima, dipoll tiap iterasi
//   - sensor cahaya : dibaca berkala tiap LIGHT_READ_INTERVAL_MS
//   - SD card       : dicatat setiap kali ada paket baru berhasil didekode
//   - WiFi          : HANYA bagian ini yang di-duty-cycle -- dinyalakan
//                      tiap WIFI_CYCLE_INTERVAL_MS untuk upload +
//                      sinkron config, lalu dimatikan lagi
class SystemManager {
public:
    static void init();     // inisialisasi satu kali: serial, I2C, radio, RTC, SD, light sensor, config
    static void loop();     // dipanggil terus-menerus dari loop() Arduino

private:
    static WeatherData lastWeather;
    static DeviceStatus status;

    static uint32_t lastLightReadMs;
    static uint32_t lastRadioWatchdogMs;
    static uint32_t lastWifiCycleMs;
    static bool     firstWifiCycleDone;   // supaya siklus WiFi pertama langsung jalan saat boot, tidak nunggu 5 menit

    static void pollRadio();              // cek & proses paket radio (non-blocking)
    static void pollLight();              // baca sensor cahaya berkala
    static void pollRadioWatchdog();      // SetRx() berkala anti-nyangkut
    static void runWifiCycleIfDue();      // cek timer & jalankan siklus WiFi kalau sudah waktunya
    static void doWifiCycle();            // isi siklus WiFi: konek -> sinkron config -> upload -> flush SD -> disconnect
};

// --------------------------- 5.14 BootManager ------------------------
class BootManager {
public:
    static void init();
    static uint32_t getBootCount();
};
