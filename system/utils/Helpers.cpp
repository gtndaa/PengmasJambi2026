#include "Helpers.h"
#include "rtc/RTCManager.h"
#include "config.h"

String Helpers::getTimestampStr(uint32_t epoch) {
    // Sederhana
    return String(epoch);
}

uint32_t Helpers::getEpochFromRTC() {
    RTCManager rtc;
    if (rtc.begin()) {
        DateTime dt = rtc.now();
        return dt.unixtime();
    }
    return 0;
}

float Helpers::mapf(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

String Helpers::toHex(uint8_t* data, uint8_t len) {
    String s;
    for (uint8_t i = 0; i < len; i++) {
        if (data[i] < 0x10) s += "0";
        s += String(data[i], HEX);
        s += " ";
    }
    return s;
}