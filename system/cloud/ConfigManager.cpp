#include "ConfigManager.h"
#include "storage/RTCMemory.h"
#include "storage/SDManager.h"
#include "utils/Logger.h"
#include <ArduinoJson.h>

void ConfigManager::setDefaults(DeviceConfig& cfg) {
    strcpy(cfg.wifiSSID, "your-ssid");
    strcpy(cfg.wifiPassword, "your-password");
    strcpy(cfg.serverURL, SERVER_URL);
    strcpy(cfg.apiKey, API_KEY);
    cfg.uploadInterval = UPLOAD_INTERVAL_MS;
    cfg.listenWindow = LISTEN_WINDOW_MS;
    cfg.timezoneOffset = TIMEZONE_OFFSET;
    cfg.configVersion = 1;
    cfg.useDeepSleep = true;
}

bool ConfigManager::load(DeviceConfig& cfg) {
    // Coba dari RTC memory dulu
    uint8_t buf[sizeof(DeviceConfig)];
    size_t len;
    if (RTCMemory::loadBuffer(buf, sizeof(buf), len) && len == sizeof(DeviceConfig)) {
        memcpy(&cfg, buf, sizeof(DeviceConfig));
        return true;
    }
    // Jika gagal, muat dari SD (jika ada)
    SDManager sd;
    if (sd.begin()) {
        String content;
        if (sd.readAll(content)) {
            DynamicJsonDocument doc(1024);
            if (deserializeJson(doc, content) == DeserializationError::Ok) {
                strcpy(cfg.wifiSSID, doc["ssid"]);
                strcpy(cfg.wifiPassword, doc["password"]);
                strcpy(cfg.serverURL, doc["server"]);
                cfg.uploadInterval = doc["uploadInterval"];
                cfg.listenWindow = doc["listenWindow"];
                cfg.timezoneOffset = doc["timezone"];
                return true;
            }
        }
    }
    // Default
    setDefaults(cfg);
    return true;
}

bool ConfigManager::save(const DeviceConfig& cfg) {
    RTCMemory::saveBuffer((uint8_t*)&cfg, sizeof(cfg));
    return true;
}

bool ConfigManager::downloadFromCloud() {
    // Stub: ambil dari cloud dan simpan
    return true;
}

bool ConfigManager::applyConfig(const DeviceConfig& cfg) {
    current = cfg;
    return save(cfg);
}

bool ConfigManager::rollback() {
    // Stub
    return false;
}