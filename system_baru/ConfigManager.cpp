#include "headers.h"
#include <ArduinoJson.h>

ConfigManager::ConfigManager() {}

bool ConfigManager::begin(const char* namespaceName) {
    ns = namespaceName;
    prefs.begin(ns.c_str(), false);
    return true;
}

bool ConfigManager::load(DeviceConfig& cfg) {
    cfg.wifiSSID = prefs.getString("wifiSSID", DEFAULT_WIFI_SSID);
    cfg.wifiPassword = prefs.getString("wifiPass", DEFAULT_WIFI_PASSWORD);
    cfg.serverURL = prefs.getString("serverURL", SERVER_URL);
    cfg.apiKey = prefs.getString("apiKey", "");
    cfg.wifiInterval = prefs.getULong("wifiInt", WIFI_CYCLE_INTERVAL_MS);
    if (cfg.wifiInterval < MIN_WIFI_INTERVAL_MS) cfg.wifiInterval = MIN_WIFI_INTERVAL_MS;
    cfg.configVersion = prefs.getUInt("cfgVersion", 0);
    LOG_DEBUG("Config loaded: wifiInterval=%lu, configVersion=%u",
               cfg.wifiInterval, (unsigned)cfg.configVersion);
    return true;
}

bool ConfigManager::save(const DeviceConfig& cfg) {
    prefs.putString("wifiSSID", cfg.wifiSSID);
    prefs.putString("wifiPass", cfg.wifiPassword);
    prefs.putString("serverURL", cfg.serverURL);
    prefs.putString("apiKey", cfg.apiKey);
    prefs.putULong("wifiInt", cfg.wifiInterval);
    prefs.putUInt("cfgVersion", cfg.configVersion);
    LOG_INFO("Config saved (configVersion=%u)", (unsigned)cfg.configVersion);
    return true;
}

bool ConfigManager::updateFromJSON(const String& json) {
    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        LOG_ERROR("JSON parse error: %s", err.c_str());
        return false;
    }
    DeviceConfig newCfg;
    load(newCfg); // ambil yang sekarang

    if (doc.containsKey("wifiSSID")) newCfg.wifiSSID = doc["wifiSSID"].as<String>();
    if (doc.containsKey("wifiPassword")) newCfg.wifiPassword = doc["wifiPassword"].as<String>();
    if (doc.containsKey("serverURL")) newCfg.serverURL = doc["serverURL"].as<String>();
    if (doc.containsKey("apiKey")) newCfg.apiKey = doc["apiKey"].as<String>();
    if (doc.containsKey("wifiInterval")) newCfg.wifiInterval = doc["wifiInterval"].as<unsigned long>();

    bool saved = save(newCfg);
    if (saved) LOG_INFO("Config updated from JSON");
    return saved;
}
