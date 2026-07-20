#include "LightSensor.h"
#include "pins.h"
#include "config.h"

bool LightSensor::begin() {
    if (sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        return true;
    }
    return false;
}

void LightSensor::update() {
    if (millis() - lastRead < interval) return;
    lastRead = millis();
    float val = sensor.readLightLevel();
    if (val >= 0) lux = val;
}

float LightSensor::getLux() const {
    return lux;
}

void LightSensor::setInterval(uint32_t ms) {
    interval = ms;
}