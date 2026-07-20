#include "SystemManager.h"
#include "core/BootManager.h"
#include "core/Scheduler.h"
#include "radio/CC1101Driver.h"
#include "radio/WeatherDecoder.h"
#include "sensors/LightSensor.h"
#include "rtc/RTCManager.h"
#include "storage/SDManager.h"
#include "cloud/WiFiManager.h"
#include "cloud/CloudAPI.h"
#include "cloud/ConfigManager.h"
#include "power/PowerManager.h"
#include "utils/Logger.h"
#include "utils/Helpers.h"
#include "config.h"

WeatherData SystemManager::lastWeather;
DeviceStatus SystemManager::status;
uint32_t SystemManager::lastUploadTime = 0;

void SystemManager::init() {
    BootManager::init();
    Logger::setOutput(&Serial);
    Serial.begin(115200);
    delay(300);

    // Inisialisasi I2C
    Wire.begin(I2C_SDA, I2C_SCL);

    // RTC
    RTCManager rtc;
    rtc.begin();

    // Light sensor
    LightSensor light;
    light.begin();

    // Radio
    CC1101Driver radio;
    if (!radio.begin()) {
        LOG_ERROR("CC1101 gagal inisialisasi");
    } else {
        LOG_INFO("CC1101 OK");
    }

    // Power
    PowerManager pm;
    pm.begin();

    // SD (opsional)
    SDManager sd;
    sd.begin();

    // Config
    DeviceConfig cfg;
    ConfigManager config;
    config.load(cfg);

    // WiFi (jika diperlukan)
    // WiFiManager wifi;
    // wifi.connect(cfg.wifiSSID, cfg.wifiPassword);

    LOG_INFO("Sistem siap.");
}

void SystemManager::run() {
    static uint32_t lastListen = 0;
    uint32_t now = millis();

    // Jadwal mendengarkan radio
    if (now - lastListen >= LISTEN_WINDOW_MS) {
        receiveWeather();
        lastListen = now;
    }

    // Baca lux secara periodik
    readLight();

    // Cek waktu upload
    if (Scheduler::isTimeToUpload()) {
        upload();
        Scheduler::resetUploadTimer();
    }

    // Simpan data ke SD (misal setiap 10 menit)
    // storeData();

    // Deep sleep jika tidak ada aktivitas
    // Scheduler::goToSleep();
}

void SystemManager::receiveWeather() {
    CC1101Driver radio;
    WeatherDecoder decoder;
    uint32_t pulses[MAX_PULSES];
    uint8_t bits[MAX_PULSES];

    radio.resetPulseBuffer();
    uint32_t start = millis();
    while (millis() - start < LISTEN_WINDOW_MS) {
        if (radio.isPacketAvailable()) {
            uint16_t count = radio.getPulseCount();
            radio.copyPulses(pulses, count);
            radio.resetPulseBuffer();

            uint16_t bitCount = decoder.pulsesToBits(pulses, count, bits, MAX_PULSES);
            if (bitCount >= 64) {
                uint8_t packet[MAX_BYTES];
                uint8_t pktLen;
                if (decoder.scanForPacket(bits, bitCount, packet, &pktLen)) {
                    if (decoder.decodePacket(packet, pktLen, lastWeather, lastWeather.rainTotal)) {
                        // Isi timestamp dan light
                        RTCManager rtc;
                        if (rtc.isOK()) {
                            lastWeather.timestamp = rtc.now().unixtime();
                        } else {
                            lastWeather.timestamp = millis() / 1000;
                        }
                        lastWeather.light = lastWeather.light; // akan diisi oleh readLight

                        LOG_INFO("Weather decoded: T=%.1f C, H=%d%%, Wind=%.1f km/h",
                                 lastWeather.temperature, lastWeather.humidity, lastWeather.windSpeed);
                        // Simpan atau upload nanti
                    }
                }
            }
            break; // keluar setelah satu paket
        }
        delay(10);
    }
    radio.watchdogReset();
}

void SystemManager::readLight() {
    LightSensor light;
    light.update();
    lastWeather.light = light.getLux();
}

void SystemManager::storeData() {
    SDManager sd;
    if (!sd.isPresent()) return;
    String line = String(lastWeather.timestamp) + ","
                + String(lastWeather.temperature) + ","
                + String(lastWeather.humidity) + ","
                + String(lastWeather.windSpeed) + ","
                + String(lastWeather.rainTotal);
    sd.appendRecord(line.c_str());
}

void SystemManager::upload() {
    if (lastWeather.timestamp == 0) return; // belum ada data

    // Baca konfigurasi untuk server
    DeviceConfig cfg;
    ConfigManager config;
    config.load(cfg);

    // WiFi
    WiFiManager wifi;
    if (!wifi.connect(cfg.wifiSSID, cfg.wifiPassword)) {
        LOG_ERROR("Gagal konek WiFi");
        return;
    }

    CloudAPI api;
    api.begin(cfg.serverURL, cfg.apiKey);
    if (api.uploadWeather(lastWeather)) {
        LOG_INFO("Upload sukses");
        // Reset rain total setelah upload? Tergantung kebutuhan
    }
    wifi.disconnect();
}