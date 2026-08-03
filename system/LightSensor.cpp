#include "headers.h"

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

float LightSensor::readOnce() {
    // Dipakai saat siklus bangun singkat: tidak menunggu jadwal `interval`,
    // langsung minta satu pembacaan agar wake time tetap pendek.
    float val = sensor.readLightLevel();
    if (val >= 0) {
        lux = val;
        lastRead = millis();
    }
    return lux;
}

void LightSensor::powerDown() {
    // Library BH1750 yang dipakai tidak meng-expose mode POWER_DOWN secara
    // publik (hanya Mode pembacaan seperti CONTINUOUS_HIGH_RES_MODE dll).
    // Tidak masalah: board ini deep-sleep total, jalur daya BH1750 ikut
    // mati bersama peripheral lain saat esp_deep_sleep_start(), jadi tidak
    // perlu perintah power-down eksplisit lewat I2C.
}