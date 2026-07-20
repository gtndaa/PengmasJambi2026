#include "CC1101Driver.h"
#include "pins.h"
#include "constants.h"

static volatile uint32_t pulseBuf[MAX_PULSES];
static volatile uint16_t pulseIdx = 0;
static volatile uint32_t lastPulseMs = 0;
static volatile bool     packetReady = false;
static volatile uint32_t risingTs = 0;

static void IRAM_ATTR gdo2ISR() {
    uint32_t now = micros();
    if (digitalRead(GDO2_PIN) == HIGH) {
        risingTs = now;
        return;
    }
    uint32_t dur = now - risingTs;
    if (packetReady) return;
    if ((dur >= PULSE_1_MIN && dur <= PULSE_1_MAX) ||
        (dur >= PULSE_0_MIN && dur <= PULSE_0_MAX)) {
        if (pulseIdx < MAX_PULSES) {
            pulseBuf[pulseIdx++] = dur;
            lastPulseMs = millis();
        }
    }
}

bool CC1101Driver::begin() {
    ELECHOUSE_cc1101.setSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CSN);
    ELECHOUSE_cc1101.setGDO(GDO0_PIN, GDO2_PIN);
    ELECHOUSE_cc1101.Init();
    if (!ELECHOUSE_cc1101.getCC1101()) return false;
    ELECHOUSE_cc1101.setMHZ(RF_FREQ_MHZ);
    ELECHOUSE_cc1101.setModulation(RF_MODULATION);
    ELECHOUSE_cc1101.setDRate(RF_DATARATE);
    ELECHOUSE_cc1101.setRxBW(RF_RXBW);
    ELECHOUSE_cc1101.SetRx();
    pinMode(GDO2_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(GDO2_PIN), gdo2ISR, CHANGE);
    return true;
}

void CC1101Driver::setReceiveMode() {
    ELECHOUSE_cc1101.SetRx();
}

bool CC1101Driver::isPacketAvailable() {
    if (packetReady) return true;
    if (pulseIdx < 64) return false;
    noInterrupts();
    uint32_t last = lastPulseMs;
    interrupts();
    if (millis() - last >= PACKET_TIMEOUT) {
        packetReady = true;
        return true;
    }
    return false;
}

uint32_t CC1101Driver::getPulseDuration() {
    // Tidak digunakan langsung, karena ISR menangani semua
    return 0;
}

void CC1101Driver::resetPulseBuffer() {
    noInterrupts();
    pulseIdx = 0;
    packetReady = false;
    interrupts();
}

uint16_t CC1101Driver::getPulseCount() {
    noInterrupts();
    uint16_t c = pulseIdx;
    interrupts();
    return c;
}

void CC1101Driver::copyPulses(uint32_t* dest, uint16_t maxCount) {
    noInterrupts();
    uint16_t c = (pulseIdx < maxCount) ? pulseIdx : maxCount;
    for (uint16_t i = 0; i < c; i++) dest[i] = pulseBuf[i];
    interrupts();
}

void CC1101Driver::enableInterrupt() {
    attachInterrupt(digitalPinToInterrupt(GDO2_PIN), gdo2ISR, CHANGE);
}

void CC1101Driver::disableInterrupt() {
    detachInterrupt(digitalPinToInterrupt(GDO2_PIN));
}

void CC1101Driver::watchdogReset() {
    ELECHOUSE_cc1101.SetRx();
}