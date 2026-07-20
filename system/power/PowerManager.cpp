#include "PowerManager.h"
#include "pins.h"
#include <esp_sleep.h>
#include <driver/adc.h>

void PowerManager::begin() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // GPIO34
}


float PowerManager::readSuperCapVoltage() {
    int val = analogRead(SUPERCAP_PIN);
    float vin = ((val / 4095.0) * 3.3) / 0.18;
    return vin;
}

void PowerManager::prepareDeepSleep(uint64_t wakeUpTimeUs) {
    esp_sleep_enable_timer_wakeup(wakeUpTimeUs);
}

void PowerManager::deepSleepNow() {
    esp_deep_sleep_start();
}

bool PowerManager::isLowPower() const {
    return readSuperCapVoltage() < 3.0;
}

void PowerManager::setSleepInterval(uint64_t us) {
    sleepInterval = us;
}