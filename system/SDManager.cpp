#include "headers.h"
#include <SPI.h>

bool SDManager::begin() {
    pinMode(CC1101_CSN, OUTPUT);
    digitalWrite(CC1101_CSN, HIGH);

    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    if (!SD.begin(SD_CS, SPI, 1000000)) {
        delay(50);
        SPI.end();
        SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
        if (!SD.begin(SD_CS, SPI, 1000000)) {
            present = false;
            return false;
        }
    }
    present = true;
    return true;
}

bool SDManager::isPresent() const { return present; }

bool SDManager::appendRecord(const char* data) {
    if (!present) return false;
    File f = SD.open(filename.c_str(), FILE_APPEND);
    if (!f) return false;
    size_t written = f.println(data);
    f.close();
    return written > 0;
}

// =====================================================================
// Antrian persisten (queue.csv) untuk data yang gagal terkirim ke server
// =====================================================================
// Format 1 baris = 1 data cuaca, urutan field sama persis dengan yang
// dikirim CloudAPI::uploadWeather() supaya gampang dipetakan ulang:
// timestamp,temperature,humidity,windSpeed,windGust,windDirection,
// windDeg,rainDelta,rainTotal,rainRaw,light,channel,batteryOk

String SDManager::encodeRecord(const WeatherData& d) {
    String s;
    s += String(d.timestamp);       s += ",";
    s += String(d.temperature, 2);  s += ",";
    s += String(d.humidity);        s += ",";
    s += String(d.windSpeed, 2);    s += ",";
    s += String(d.windGust, 2);     s += ",";
    s += String(d.windDirection);   s += ",";
    s += String(d.windDeg, 2);      s += ",";
    s += String(d.rainDelta, 2);    s += ",";
    s += String(d.rainTotal, 2);    s += ",";
    s += String(d.rainRaw);         s += ",";
    s += String(d.light, 2);        s += ",";
    s += String(d.channel);         s += ",";
    s += String(d.batteryOk ? 1 : 0);
    return s;
}

bool SDManager::decodeRecord(const String& line, WeatherData& out) {
    String parts[13];
    int start = 0;
    for (int i = 0; i < 13; i++) {
        int comma = line.indexOf(',', start);
        if (i < 12) {
            if (comma < 0) return false; // baris rusak/tidak lengkap
            parts[i] = line.substring(start, comma);
            start = comma + 1;
        } else {
            parts[i] = line.substring(start);
        }
    }

    out.timestamp     = (uint32_t)parts[0].toInt();
    out.temperature   = parts[1].toFloat();
    out.humidity       = (uint8_t)parts[2].toInt();
    out.windSpeed      = parts[3].toFloat();
    out.windGust       = parts[4].toFloat();
    out.windDirection  = (uint8_t)parts[5].toInt();
    out.windDeg        = parts[6].toFloat();
    out.rainDelta      = parts[7].toFloat();
    out.rainTotal      = parts[8].toFloat();
    out.rainRaw        = (uint16_t)parts[9].toInt();
    out.light          = parts[10].toFloat();
    out.channel        = (uint8_t)parts[11].toInt();
    out.batteryOk      = parts[12].toInt() != 0;
    out.sensorId       = 0;
    return true;
}

bool SDManager::queuePush(const WeatherData& d) {
    if (!present) return false;
    File f = SD.open(queueFilename.c_str(), FILE_APPEND);
    if (!f) return false;
    size_t written = f.println(encodeRecord(d));
    f.close();
    return written > 0;
}

uint16_t SDManager::queueCount() {
    if (!present) return 0;
    File f = SD.open(queueFilename.c_str(), FILE_READ);
    if (!f) return 0;
    uint16_t n = 0;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) n++;
    }
    f.close();
    return n;
}

void SDManager::queueFlush(CloudAPI& api, uint16_t& outSent, uint16_t& outRemaining,
                            uint16_t maxRecords, const char* capBattField) {
    outSent = 0;
    outRemaining = 0;
    if (!present) return;

    // Proses SATU record tertua per iterasi, dan ketika record itu sukses terkirim,
    // langsung tulis ulang queue.csv tanpa record tsb saat itu juga (bukan di akhir). 
    for (uint16_t i = 0; (maxRecords == 0 || i < maxRecords); i++) {
        File f = SD.open(queueFilename.c_str(), FILE_READ);
        if (!f) break; // tidak ada antrian sama sekali

        String firstLine;
        bool haveFirst = false;
        while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (line.length() == 0) continue;
            firstLine = line;
            haveFirst = true;
            break;
        }
        if (!haveFirst) { f.close(); break; } // antrian sudah kosong

        WeatherData d;
        bool parsed = decodeRecord(firstLine, d);
        bool uploaded = parsed && api.uploadWeather(d, capBattField);

        if (!uploaded) {
            // Gagal (WiFi/server bermasalah, atau baris rusak) -> hentikan
            // batch ini, jangan paksa lanjut ke baris berikutnya kalau
            // sinyalnya memang lagi jelek. Baris ini tidak disentuh sama
            // sekali di file, dicoba lagi siklus berikutnya.
            f.close();
            break;
        }

        outSent++;

        // Sukses -> commit SEKARANG: tulis ulang queue.csv tanpa baris
        // pertama ini (baris² sisanya disalin apa adanya).
        const char* tmpName = "/queue.tmp";
        SD.remove(tmpName);
        File tmp = SD.open(tmpName, FILE_WRITE);
        if (tmp) {
            while (f.available()) {
                String rest = f.readStringUntil('\n');
                rest.trim();
                if (rest.length() > 0) tmp.println(rest);
            }
            tmp.close();
            f.close();
            SD.remove(queueFilename.c_str());
            SD.rename(tmpName, queueFilename.c_str());
        } else {
            f.close();
        }

        yield(); // beri jatah CPU ke task WiFi/lwIP tiap record
    }

    outRemaining = queueCount();
}