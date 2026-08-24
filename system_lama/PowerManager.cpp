#include "headers.h"
#include <esp_sleep.h>
#include <driver/adc.h>

void PowerManager::begin() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // GPIO34
}


float PowerManager::readSuperCapVoltage() const {
    int val = analogRead(SUPERCAP_PIN);
    float vin = ((val / 4095.0) * 3.3) / 0.18;
    return vin;
}

float PowerManager::readBatteryVoltage() const {
    // Board ini tidak punya pembagi tegangan baterai terpisah dari
    // supercap, jadi dipetakan ke pin/rumus yang sama. Ganti BATTERY_PIN
    // dan faktor pembagi di headers.h jika board revisi berikutnya
    // punya jalur ADC baterai sendiri.
    int val = analogRead(BATTERY_PIN);
    float vin = ((val / 4095.0) * 3.3) / 0.18;
    return vin;
}

void PowerManager::prepareDeepSleep(uint64_t wakeUpTimeUs) {
    esp_sleep_enable_timer_wakeup(wakeUpTimeUs);

    // PENTING: CC1101_CSN (GPIO5) BUKAN RTC-capable GPIO di ESP32 --
    // artinya domain RTC yang tetap aktif selama deep sleep TIDAK BISA
    // secara default mempertahankan level output pin ini. Begitu ESP32
    // masuk deep sleep, GPIO5 jadi FLOATING (high-Z) sepanjang durasi
    // tidur (bisa puluhan detik), bukan didorong HIGH (deselect) secara
    // aktif seperti saat CPU jalan normal. Pin CS yang floating lama
    // itu rawan kepancing noise/EMI (apalagi berbagi bus SPI dengan SD
    // card di SCK/MOSI/MISO yang sama) dan bisa "bergetar" ke LOW
    // sesaat -- CC1101 bisa salah mengira ada awal transaksi SPI parsial
    // masuk, merusak state internalnya. Ini kemungkinan besar penyebab
    // pola "sukses sekali di awal (CS masih aktif didorong CPU), lalu
    // rusak permanen setelah siklus deep-sleep pertama" -- karena mode
    // kontinu (tanpa deep sleep sama sekali, CS selalu aktif didorong)
    // terbukti tidak pernah mengalami masalah ini.
    //
    // Fix: paksa CS HIGH (deselect) lalu KUNCI level itu via
    // gpio_hold_en() supaya tetap terjaga HIGH sepanjang deep sleep,
    // bukan floating. gpio_deep_sleep_hold_en() diperlukan sebagai
    // saklar global supaya hold per-pin ini benar-benar berlaku untuk
    // DEEP sleep (bukan cuma light sleep).
    pinMode(CC1101_CSN, OUTPUT);
    digitalWrite(CC1101_CSN, HIGH);
    gpio_hold_en((gpio_num_t)CC1101_CSN);
    gpio_deep_sleep_hold_en();
}

void PowerManager::deepSleepNow() {
    esp_deep_sleep_start();
}