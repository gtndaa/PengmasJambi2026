#pragma once
#include <Arduino.h>
#include <BH1750.h>

class LightSensor {
public:
    bool begin();
    void update();          // baca lux jika interval terlewati
    float getLux() const;
    void setInterval(uint32_t ms); // default LUX_INTERVAL
private:
    BH1750 sensor;
    float lux = 0.0f;
    uint32_t lastRead = 0;
    uint32_t interval = LUX_INTERVAL;
};