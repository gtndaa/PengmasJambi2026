#include "headers.h"

// =====================================================================
// ESP32 deep sleep me-reset seluruh RAM (kecuali RTC memory), sehingga
// setup() akan berjalan lagi dari awal setiap kali MCU bangun. Karena
// itu seluruh "siklus kerja" (dengar radio, baca lux, upload) dijalankan
// SEKALI di sini, lalu MCU langsung dikirim tidur lagi. Tidak ada
// loop() yang berputar terus-menerus di mode hemat daya ini.
// =====================================================================

void setup() {
    SystemManager::init();
    SystemManager::run();
    Scheduler::goToSleep();   // durasi otomatis disinkronkan ke jadwal paket ~48s
    // Tidak akan sampai sini: esp_deep_sleep_start() memutus eksekusi.
}

void loop() {
    // Sengaja kosong. Aktifkan blok di bawah hanya untuk mode debugging
    // tanpa deep sleep (misal saat mengembangkan protokol/decoder baru):
    //
    // SystemManager::run();
    // delay(1000);
}
