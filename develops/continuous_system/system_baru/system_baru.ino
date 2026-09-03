#include "headers.h"

void setup() {
    SystemManager::init();
}

void loop() {
    SystemManager::loop();
    // Sengaja tidak ada delay() besar di sini -- semua timing internal
    // (baca cahaya, watchdog radio, siklus WiFi) sudah non-blocking
    // berbasis millis() di dalam SystemManager::loop(). delay(1) hanya
    // untuk kasih jatah kecil ke task WiFi/lwIP di background.
    delay(1);
}
