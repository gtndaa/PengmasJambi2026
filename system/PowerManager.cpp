#include "headers.h"
#include <esp_sleep.h>

void PowerManager::begin() {
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
}

static int readAdcAveraged(int pin) {
    analogRead(pin);
    delayMicroseconds(100);
    const uint8_t SAMPLES = 16;
    long sum = 0;
    for (uint8_t i = 0; i < SAMPLES; i++) {
        sum += analogRead(pin);
        delayMicroseconds(100);
    }
    return (int)(sum / SAMPLES);
}

static float rawToVoltage(int raw) {
    float vadc = (raw / 4095.0f) * 3.3f;
    float vin  = vadc / 0.18f;                                   // rasio divider nominal 100k/22k
    vin = vin * SUPERCAP_CAL_SLOPE + SUPERCAP_CAL_OFFSET;         // koreksi hasil kalibrasi multimeter
    if (vin < 0) vin = 0;
    return vin;
}

float PowerManager::readSuperCapVoltage() const {
    int val = readAdcAveraged(SUPERCAP_PIN);
    float vin = rawToVoltage(val);
    LOG_DEBUG("ADC supercap raw=%d Vin=%.2f", val, vin);
    return vin;
}

float PowerManager::readBatteryVoltage() const {
    int val = readAdcAveraged(BATTERY_PIN);
    float vin = rawToVoltage(val);
    return vin;
}

SuperCapState PowerManager::updateSuperCapState(float currentV) const {
    float peak     = RTCMemory::getCapPeakVoltage();
    float trough   = RTCMemory::getCapTroughVoltage();
    SuperCapState state = RTCMemory::getCapState();
    uint8_t riseCount    = RTCMemory::getCapRiseConfirmCount();

    bool firstReading = (peak <= 0.01f);
    if (firstReading) {
        peak = currentV;
        trough = 0;
        state = (currentV >= SUPERCAP_FULL_V) ? CAP_STATE_STANDBY_PLN : CAP_STATE_CHARGING;
        riseCount = 0;
    } else if (state == CAP_STATE_UNKNOWN || state == CAP_STATE_CHARGING || state == CAP_STATE_STANDBY_PLN) {
        // Charging/standby: naik terus dianggap normal. Turun cukup
        // jauh dari puncak berarti PLN putus, mulai pakai supercap.
        if (currentV > peak) peak = currentV;

        if (currentV <= peak - SUPERCAP_DROP_HYSTERESIS) {
            state = CAP_STATE_ON_CAP;
            trough = currentV;
            riseCount = 0;
        } else {
            state = (peak >= SUPERCAP_FULL_V) ? CAP_STATE_STANDBY_PLN : CAP_STATE_CHARGING;
        }
    } else {
        // ON_CAP/LOW/CRITICAL. Recovery dilihat dari kenaikan terhadap
        // titik terendah (trough).
        if (trough <= 0.01f || currentV < trough) trough = currentV;

        bool recovering = (currentV >= trough + SUPERCAP_RECOVER_MARGIN);
        riseCount = recovering ? (riseCount + 1) : 0;

        if (riseCount >= SUPERCAP_RISE_CONFIRM_N) {
            peak = currentV;
            trough = 0;
            state = (currentV >= SUPERCAP_FULL_V) ? CAP_STATE_STANDBY_PLN : CAP_STATE_CHARGING;
            riseCount = 0;
        } else if (currentV <= SUPERCAP_CRITICAL_V) {
            state = CAP_STATE_CRITICAL;
        } else if (currentV <= SUPERCAP_LOW_V) {
            state = CAP_STATE_LOW;
        } else {
            state = CAP_STATE_ON_CAP;
        }
    }

    RTCMemory::setCapPeakVoltage(peak);
    RTCMemory::setCapTroughVoltage(trough);
    RTCMemory::setCapLastVoltage(currentV);
    RTCMemory::setCapState(state);
    RTCMemory::setCapRiseConfirmCount(riseCount);

    return state;
}

const char* PowerManager::superCapStateToStr(SuperCapState s) {
    switch (s) {
        case CAP_STATE_CHARGING:    return "CHARGING";
        case CAP_STATE_STANDBY_PLN: return "STANDBY_PLN";
        case CAP_STATE_ON_CAP:      return "ON_CAP";
        case CAP_STATE_LOW:         return "LOW";
        case CAP_STATE_CRITICAL:    return "CRITICAL";
        default:                    return "UNKNOWN";
    }
}

const char* PowerManager::superCapStateToBattField(SuperCapState s) {
    switch (s) {
        case CAP_STATE_LOW:      return "LOW";
        case CAP_STATE_CRITICAL: return "CRIT";
        case CAP_STATE_ON_CAP:   return "ON_CAP";
        default:                 return "OK"; // charging, standby, atau unknown dianggap aman
    }
}

void PowerManager::prepareDeepSleep(uint64_t wakeUpTimeUs) {
    esp_sleep_enable_timer_wakeup(wakeUpTimeUs);
}

void PowerManager::deepSleepNow() {
    esp_deep_sleep_start();
}