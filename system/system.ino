#include "headers.h"

void setup() {
    SystemManager::init();
    SystemManager::run();
    Scheduler::goToSleep();
}

void loop() {
    // SystemManager::run();
    // delay(1000);
}
