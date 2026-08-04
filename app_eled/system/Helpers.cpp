#include "headers.h"

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

String Helpers::epochToDateTimeStr(uint32_t epoch) {
    if (epoch == 0) return "1970-01-01 00:00:00";
    DateTime dt(epoch);
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
        dt.year(), dt.month(), dt.day(),
        dt.hour(), dt.minute(), dt.second()
    );
    return String(buf);
}

const char* Helpers::windDirToStr(uint8_t dirIndex) {
    static const char* DIRS[16] = {
        "N", "NNE", "NE", "ENE",
        "E", "ESE", "SE", "SSE",
        "S", "SSW", "SW", "WSW",
        "W", "WNW", "NW", "NNW"
    };
    if (dirIndex > 15) dirIndex = 0;
    return DIRS[dirIndex];
}