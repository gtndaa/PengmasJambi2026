#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "models/DeviceConfig.h"   // buat MQTT

class ConfigManager {
public:
    ConfigManager();
    bool begin(const char* namespaceName = "appconfig");
    
    // Muat/simpan konfigurasi ke struct DeviceConfig
    bool load(DeviceConfig& cfg);
    bool save(const DeviceConfig& cfg);
    void resetToDefault();
    
    // Update dari JSON string (dari cloud)
    bool updateFromJSON(const String& json);

private:
    Preferences prefs;
    String ns;
};