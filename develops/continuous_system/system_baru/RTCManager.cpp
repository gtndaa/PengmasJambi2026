#include "headers.h"
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
        // PENTING: ini yang paling mungkin jadi sumber "RTC selalu
        // ketinggalan waktu" walau sudah pernah di-adjust manual --
        // setiap kali lostPower() balik true (paling umum karena baterai
        // cadangan CR2032 di modul DS3231 lemah/habis/lepas kontak, atau
        // sempat ada dip tegangan sekilas di VCC saat wake dari deep
        // sleep), baris di bawah ini MENIMPA waktu RTC dengan waktu
        // KOMPILASI firmware (__DATE__/__TIME__) -- bukan waktu sekarang.
        // Kalau ini terpicu berulang di banyak wake cycle (bukan cuma
        // sekali), adjust manual manapun yang pernah dilakukan akan
        // ketimpa lagi begitu lostPower() true lagi. Log ini WAJIB
        // dipantau: kalau baris WARN ini muncul di tiap/hampir tiap
        // wake, itu indikasi kuat baterai backup DS3231 bermasalah,
        // bukan soal firmware semata.
        LOG_WARN("RTC lostPower()=true -> waktu di-reset ke waktu KOMPILASI "
                  "(%s %s), BUKAN waktu sekarang. Cek baterai backup CR2032 "
                  "di modul DS3231 kalau ini sering muncul.",
                  __DATE__, __TIME__);
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

bool RTCManager::syncFromNTP(uint32_t timeoutMs) {
    if (!ok) return false;

    if (WiFi.status() != WL_CONNECTED) {
        LOG_WARN("syncFromNTP() dipanggil tanpa WiFi konek, dibatalkan");
        return false;
    }

    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC,
               NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

    static const time_t MIN_PLAUSIBLE_EPOCH = 1704067200; // 2024-01-01 00:00:00 UTC
    time_t nowSec = 0;
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        time(&nowSec);
        if (nowSec >= MIN_PLAUSIBLE_EPOCH) break;
        delay(200);
    }

    if (nowSec < MIN_PLAUSIBLE_EPOCH) {
        LOG_WARN("NTP sync gagal/tidak valid (epoch=%ld, timeout %lu ms) -- jaringan mungkin "
                  "blokir UDP/123, RTC dibiarkan seperti sebelumnya (tidak di-fallback)",
                  (long)nowSec, (unsigned long)timeoutMs);
        return false;
    }

    uint32_t localEpoch = (uint32_t)nowSec + NTP_GMT_OFFSET_SEC;
    DateTime ntpNow(localEpoch);
    rtc.adjust(ntpNow);
    LOG_INFO("RTC berhasil di-sync dari NTP: %04d-%02d-%02d %02d:%02d:%02d (WIB)",
              ntpNow.year(), ntpNow.month(), ntpNow.day(),
              ntpNow.hour(), ntpNow.minute(), ntpNow.second());
    return true;
}

void RTCManager::markNeedResync() {
    Preferences p;
    if (!p.begin("rtcsync", false)) return;
    p.putBool("need", true);
    p.end();
}

bool RTCManager::consumeNeedResyncFlag() {
    Preferences p;
    if (!p.begin("rtcsync", false)) return false; // read-write, namespace dibuat kalau belum ada
    bool need = p.getBool("need", false);
    if (need) p.remove("need");
    p.end();
    return need;
}