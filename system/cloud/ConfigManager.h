#pragma once
#include <Arduino.h>
#include "models/DeviceConfig.h"

class ConfigManager {
public:
    bool load(DeviceConfig& cfg);          // muat dari RTC/SD
    bool save(const DeviceConfig& cfg);
    bool downloadFromCloud();              // ambil dari CloudAPI
    bool applyConfig(const DeviceConfig& cfg);
    void setDefaults(DeviceConfig& cfg);
    bool rollback();                       // pakai konfigurasi sebelumnya
private:
    DeviceConfig current;
};