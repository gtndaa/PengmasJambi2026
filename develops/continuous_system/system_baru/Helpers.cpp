#include "headers.h"

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