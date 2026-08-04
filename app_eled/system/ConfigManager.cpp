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
    cfg.mqttBroker = prefs.getString("mqttBroker", "");
    cfg.mqttPort = prefs.getUShort("mqttPort", 1883);
    cfg.mqttClientId = prefs.getString("mqttClientId", "");
    cfg.mqttUsername = prefs.getString("mqttUser", "");
    cfg.mqttPassword = prefs.getString("mqttPass", "");
    cfg.uploadInterval = prefs.getULong("uploadInt", 60000);
    cfg.listenWindow = prefs.getULong("listenWin", 30000);
    cfg.sleepInterval = prefs.getULong("sleepInt", SLEEP_INTERVAL_MS);
    cfg.useDeepSleep = prefs.getBool("useDeepSlp", true);
    cfg.configVersion = prefs.getUInt("cfgVersion", 0);
    LOG_DEBUG("Config loaded: uploadInterval=%lu, listenWindow=%lu, configVersion=%u",
               cfg.uploadInterval, cfg.listenWindow, (unsigned)cfg.configVersion);
    return true;
}

bool ConfigManager::save(const DeviceConfig& cfg) {
    prefs.putString("wifiSSID", cfg.wifiSSID);
    prefs.putString("wifiPass", cfg.wifiPassword);
    prefs.putString("serverURL", cfg.serverURL);
    prefs.putString("apiKey", cfg.apiKey);
    prefs.putString("mqttBroker", cfg.mqttBroker);
    prefs.putUShort("mqttPort", cfg.mqttPort);
    prefs.putString("mqttClientId", cfg.mqttClientId);
    prefs.putString("mqttUser", cfg.mqttUsername);
    prefs.putString("mqttPass", cfg.mqttPassword);
    prefs.putULong("uploadInt", cfg.uploadInterval);
    prefs.putULong("listenWin", cfg.listenWindow);
    prefs.putULong("sleepInt", cfg.sleepInterval);
    prefs.putBool("useDeepSlp", cfg.useDeepSleep);
    prefs.putUInt("cfgVersion", cfg.configVersion);
    LOG_INFO("Config saved (configVersion=%u)", (unsigned)cfg.configVersion);
    return true;
}

void ConfigManager::resetToDefault() {
    DeviceConfig defaultCfg;
    save(defaultCfg);
    LOG_INFO("Config reset to default");
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
    if (doc.containsKey("mqttBroker")) newCfg.mqttBroker = doc["mqttBroker"].as<String>();
    if (doc.containsKey("mqttPort")) newCfg.mqttPort = doc["mqttPort"].as<uint16_t>();
    if (doc.containsKey("mqttClientId")) newCfg.mqttClientId = doc["mqttClientId"].as<String>();
    if (doc.containsKey("mqttUsername")) newCfg.mqttUsername = doc["mqttUsername"].as<String>();
    if (doc.containsKey("mqttPassword")) newCfg.mqttPassword = doc["mqttPassword"].as<String>();
    if (doc.containsKey("uploadInterval")) newCfg.uploadInterval = doc["uploadInterval"].as<unsigned long>();
    if (doc.containsKey("listenWindow")) newCfg.listenWindow = doc["listenWindow"].as<unsigned long>();

    bool saved = save(newCfg);
    if (saved) LOG_INFO("Config updated from JSON");
    return saved;
}