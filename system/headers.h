#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <BH1750.h>
#include <RTClib.h>
#include <SmartRC_CC1101.h>   // library sebenarnya bernama ini, bukan ELECHOUSE_CC1101_SRC_DRV.h
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <functional>
#include <esp_sleep.h>
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
#define LISTEN_WINDOW_MS    30000      // durasi mendengar radio saat SINKRONISASI AWAL (belum tahu jadwal sensor)
#define SLEEP_INTERVAL_MS   300000     // fallback deep sleep jika radio tidak pernah sinkron (5 menit)
#define LUX_INTERVAL        2000       // ms antar pembacaan lux

#define TIMEZONE_OFFSET     25200

// ---- KONFIGURASI DUTY-CYCLE BERBASIS ANALISIS PAKET (48 detik) -------
// Dari analisis log rtl_433-style: sensor cuaca (id 0x22 ch4) mengirim
// paket kira-kira setiap 48 detik, kadang meleset (miss 1-3 siklus).
// Strategi: setelah paket pertama berhasil didekode & timestamp RTC
// diketahui, ESP32 memprediksi waktu paket berikutnya dan hanya
// bangun sesaat sebelum itu (guard window), lalu deep sleep lagi.
#define TX_PERIOD_MS            48000UL   // interval nominal transmisi sensor
#define TX_JITTER_GUARD_MS      4000UL    // toleransi jitter di kedua sisi window
#define WAKE_BEFORE_MS           1500UL   // bangun sedikit lebih awal dari prediksi
#define MAX_MISSED_CYCLES         3       // setelah sekian kali miss beruntun -> re-sync (dengar penuh 1 periode)
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
#define ENDPOINT_SENSOR_POST   "/postsensordata"   // POST data cuaca (SensorData model)
#define ENDPOINT_SENSOR_GET    "/sensordata"       // GET data cuaca terbaru
#define ENDPOINT_WIFI_GET      "/wifi"             // GET konfigurasi wifi terbaru (WifiConfig model)

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

#define RAIN_UNINIT         0xFF

#define CRC_POLY            0x31
#define CRC_INIT            0x00

// SD Card
#define SD_MAX_RECORDS      1000
#define SD_FILENAME         "/weather.log"

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
    uint8_t  rainRaw;            // counter mentah
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
    String mqttBroker;
    uint16_t mqttPort = 1883;
    String mqttClientId;
    String mqttUsername;
    String mqttPassword;
    unsigned long uploadInterval = UPLOAD_INTERVAL_MS;   // ms
    unsigned long listenWindow = LISTEN_WINDOW_MS;        // ms
};

// 4.4 WifiCredentials
// Struct minimal, hanya ssid + password. Dipakai khusus untuk pertukaran
// data dengan endpoint GET /wifi (backend WifiConfig model). Tidak ada
// field lain karena URL server dan konfigurasi lain tidak pernah diatur
// dari jarak jauh lewat jalur ini.
struct WifiCredentials {
    String ssid;
    String password;
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

    // ---- akumulasi rain, tetap konsisten walau deep sleep ----
    static uint8_t getRainCounterPrev();
    static void setRainCounterPrev(uint8_t v);
    static float getRainAccumulated();
    static void setRainAccumulated(float v);

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
        uint8_t rainCounterPrev;
        float   rainAccumulated;
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
private:
    RTC_DS3231 rtc;
    bool ok = false;
};

// --------------------------- 5.4 SDManager ----------------------------
class SDManager {
public:
    bool begin();
    bool isPresent() const;
    bool appendRecord(const char* data);      // tambahkan baris ke file
    bool readAll(String& content);
    bool deleteFile();
    bool flush();
private:
    bool present = false;
    File file;
    String filename = SD_FILENAME;
};

// --------------------------- 5.5 LightSensor --------------------------
class LightSensor {
public:
    bool begin();
    void update();          // baca lux jika interval terlewati
    float readOnce();       // baca sekali langsung (dipakai saat wake singkat)
    float getLux() const;
    void setInterval(uint32_t ms); // default LUX_INTERVAL
    void powerDown();       // matikan sensor sebelum deep sleep
private:
    BH1750 sensor;
    float lux = 0.0f;
    uint32_t lastRead = 0;
    uint32_t interval = LUX_INTERVAL;
};

// --------------------------- 5.6 CC1101Driver -------------------------
class CC1101Driver {
public:
    bool begin();
    void setReceiveMode();
    bool isPacketAvailable();          // periksa GDO2/ interrupt
    uint32_t getPulseDuration();       // baca durasi pulsa dari ISR
    void resetPulseBuffer();
    uint16_t getPulseCount();
    void copyPulses(uint32_t* dest, uint16_t maxCount);
    void enableInterrupt();
    void disableInterrupt();
    void watchdogReset();              // panggil SetRx() secara periodik
    void sleepRadio();                 // idle-kan CC1101 sebelum deep sleep (hemat daya)
};

// --------------------------- 5.7 WeatherDecoder ----------------------
class WeatherDecoder {
public:
    // Mengubah array pulsa menjadi bit (0/1)
    uint16_t pulsesToBits(uint32_t* pulses, uint16_t count, uint8_t* bits, uint16_t maxBits);

    // Mencari paket valid di dalam aliran bit
    bool scanForPacket(uint8_t* bits, uint16_t bitCount, uint8_t* out, uint8_t* outLen);

    // Mendekode paket menjadi WeatherData (termasuk rain delta dan akumulasi)
    bool decodePacket(uint8_t* packet, uint8_t len, WeatherData& data, float& rainAccumulated);

    // Hitung CRC-8 (polynomial 0x31)
    uint8_t crc8(uint8_t* data, uint8_t len);

    // Reset state rain counter (dipanggil saat boot)
    void resetRainCounter(uint8_t initialCounter = RAIN_UNINIT);

    // Muat/simpan state rain counter dari/ke RTC memory (persist lintas deep-sleep)
    void loadRainStateFromRTC();
    void saveRainStateToRTC();

private:
    uint8_t rainCounterPrev = RAIN_UNINIT;
    float rainAccumulated = 0.0f;
};

// --------------------------- 5.8 PowerManager -------------------------
class PowerManager {
public:
    void begin();
    float readBatteryVoltage() const;      // volt
    float readSuperCapVoltage() const;     // volt
    void prepareDeepSleep(uint64_t wakeUpTimeUs);
    void deepSleepNow();
    bool isLowPower() const;
    void setSleepInterval(uint64_t us);
private:
    uint64_t sleepInterval = 0;
    uint32_t lastWake = 0;
};

// --------------------------- 5.9 CloudAPI -----------------------------
typedef std::function<void(const String& configJSON)> ConfigCallback;

class CloudAPI {
public:
    CloudAPI(WiFiClient& client);

    bool begin(const char* serverURL,
               const char* apiKey,
               const char* mqttBroker = nullptr,
               uint16_t mqttPort = 1883,
               const char* mqttClientId = nullptr,
               const char* mqttUsername = nullptr,
               const char* mqttPassword = nullptr);

    bool connectMQTT();
    bool disconnectMQTT();
    bool isMQTTConnected() const;

    bool uploadWeather(const WeatherData& data);
    bool uploadStatus(const DeviceStatus& status);
    bool getConfig(String& configJSON);

    // Ambil kredensial wifi terbaru dari endpoint GET /wifi (backend
    // WifiConfig model). Hanya berisi ssid + password.
    bool fetchWifiCredentials(WifiCredentials& creds);

    bool subscribe(const char* topic);
    void setConfigCallback(ConfigCallback cb);
    void loop();

private:
    String server;
    String apiKey;
    mutable PubSubClient mqttClient;
    String mqttClientId;
    String mqttUsername;
    String mqttPassword;
    ConfigCallback configCb;
    static CloudAPI* instance;
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    void handleMessage(char* topic, byte* payload, unsigned int length);
};

// --------------------------- 5.10 ConfigManager ------------------------
class ConfigManager {
public:
    ConfigManager();
    bool begin(const char* namespaceName = "wscfg");
    bool load(DeviceConfig& cfg);
    bool save(const DeviceConfig& cfg);
    void resetToDefault();
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

    bool isConnected() const;
    String getLocalIP() const;
    int getRSSI() const;
    void disconnect();
};

// --------------------------- 5.12 Helpers ----------------------------
class Helpers {
public:
    static String getTimestampStr(uint32_t epoch);
    static uint32_t getEpochFromRTC();   // memerlukan RTCManager
    static float mapf(float x, float in_min, float in_max, float out_min, float out_max);
    static String toHex(uint8_t* data, uint8_t len);

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
    static void readLight();
    static void storeData();
    static void upload();
    static uint32_t computeNextSleepMs(); // dihitung setelah run(), dipakai Scheduler::goToSleep()
private:
    static WeatherData lastWeather;
    static DeviceStatus status;
    static uint32_t lastUploadTime;
    static bool packetReceivedThisCycle;
};

// --------------------------- 5.14 Scheduler --------------------------
class Scheduler {
public:
    static void setNextWake(uint32_t intervalMs);
    static void goToSleep();                 // pakai durasi hasil SystemManager::computeNextSleepMs()
    static void goToSleep(uint32_t sleepMs);  // durasi eksplisit
    static bool isTimeToUpload();
    static bool isTimeToListen();
    static void resetListenTimer();
    static void resetUploadTimer();
private:
    static uint32_t lastListenTime;
    static uint32_t lastUploadTime;
};

// --------------------------- 5.15 BootManager ------------------------
class BootManager {
public:
    static void init();
    static void printSystemInfo();
    static bool isFirstBoot();
    static uint32_t getBootCount();
};

