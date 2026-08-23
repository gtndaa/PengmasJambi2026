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
    if (!ready) {
        LOG_WARN("BH1750 belum siap (begin() gagal), lux tidak dibaca");
        return lux;
    }

    // Guard settle time HANYA relevan tepat setelah begin() -- di mode
    // kontinu, readOnce() dipanggil berkala tiap LIGHT_READ_INTERVAL_MS
    // sepanjang runtime, jadi guard ini praktis hanya berlaku sekali
    // saja di awal (bacaan-bacaan berikutnya sudah pasti lewat dari
    // 180ms sejak begin()).
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
