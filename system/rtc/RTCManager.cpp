#include "RTCManager.h"
#include "pins.h"
#include "config.h"

static const char* DAYS[] = {
    "Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu"
};

bool RTCManager::begin() {
    if (!rtc.begin()) {
        ok = false;
        return false;
    }
    ok = true;
    if (rtc.lostPower()) {
        // Set ke waktu kompilasi
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    return true;
}

bool RTCManager::isOK() const { return ok; }

DateTime RTCManager::now() {
    return rtc.now();
}

float RTCManager::getTemperature() {
    return rtc.getTemperature();
}

String RTCManager::dateTimeStr() {
    if (!ok) return "RTC ERROR";
    DateTime dt = rtc.now();
    char buf[40];
    snprintf(buf, sizeof(buf), "%s, %02d/%02d/%04d %02d:%02d:%02d",
        DAYS[dt.dayOfTheWeek()],
        dt.day(), dt.month(), dt.year(),
        dt.hour(), dt.minute(), dt.second()
    );
    return String(buf);
}

String RTCManager::isoStr() {
    if (!ok) return "1970-01-01T00:00:00";
    DateTime dt = rtc.now();
    char buf[25];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
        dt.year(), dt.month(), dt.day(),
        dt.hour(), dt.minute(), dt.second()
    );
    return String(buf);
}

void RTCManager::adjust(const DateTime& dt) {
    if (ok) rtc.adjust(dt);
}