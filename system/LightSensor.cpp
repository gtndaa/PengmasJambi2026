#include "headers.h"

bool LightSensor::begin() {
    ready = sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
    if (ready) {
        readySince = millis();
    } else {
        LOG_WARN("BH1750 tidak terdeteksi (I2C SDA=%d SCL=%d, alamat default 0x23). "
                  "Cek pin ADDR sensor (harus low/floating), wiring SDA/SCL, dan VCC/GND.",
                  I2C_SDA, I2C_SCL);
    }
    return ready;
}

bool LightSensor::isReady() const {
    return ready;
}

float LightSensor::readOnce() {
    // Dipakai saat siklus bangun singkat: tidak menunggu jadwal `interval`,
    // langsung minta satu pembacaan agar wake time tetap pendek.
    if (!ready) {
        LOG_WARN("BH1750 belum siap (begin() gagal), lux tidak dibaca");
        return lux;
    }

    static const uint32_t BH1750_FIRST_READ_SETTLE_MS = 180;
    uint32_t elapsed = millis() - readySince;
    if (elapsed < BH1750_FIRST_READ_SETTLE_MS) {
        delay(BH1750_FIRST_READ_SETTLE_MS - elapsed);
    }

    float val = sensor.readLightLevel();
    if (val >= 0) {
        lux = val;
    } else {
        LOG_WARN("Gagal membaca BH1750 (readLightLevel() mengembalikan nilai negatif)");
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