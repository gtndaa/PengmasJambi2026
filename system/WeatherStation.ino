#include "core/SystemManager.h"

void setup() {
    SystemManager::init();
}

void loop() {
    SystemManager::run();
    // jika menggunakan deep sleep, panggil Scheduler::goToSleep() di sini
    // delay(1000); // hanya untuk debugging
}