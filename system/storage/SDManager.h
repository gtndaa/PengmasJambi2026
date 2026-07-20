#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SD.h>

class SDManager {
public:
    bool begin();
    bool isPresent() const;
    bool appendRecord(const char* data);      // tambahkan baris ke file
    bool readAll(String& content);
    bool deleteFile();
    bool flush();
private:
    bool present = false;
    File file;
    String filename = SD_FILENAME;
};