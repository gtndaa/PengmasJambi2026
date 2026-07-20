#pragma once
#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

class CC1101Driver {
public:
    bool begin();
    void setReceiveMode();
    bool isPacketAvailable();          // periksa GDO2/ interrupt
    uint32_t getPulseDuration();       // baca durasi pulsa dari ISR
    void resetPulseBuffer();
    uint16_t getPulseCount();
    void copyPulses(uint32_t* dest, uint16_t maxCount);
    void enableInterrupt();
    void disableInterrupt();
    void watchdogReset();              // panggil SetRx() secara periodik
};