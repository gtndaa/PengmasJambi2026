#include "SDManager.h"
#include "pins.h"
#include "constants.h"
#include <SPI.h>

bool SDManager::begin() {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) {
        present = false;
        return false;
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

bool SDManager::readAll(String& content) {
    if (!present) return false;
    File f = SD.open(filename.c_str(), FILE_READ);
    if (!f) return false;
    content = f.readString();
    f.close();
    return true;
}

bool SDManager::deleteFile() {
    if (!present) return false;
    return SD.remove(filename.c_str());
}

bool SDManager::flush() {
    // Tidak ada operasi khusus untuk SD
    return true;
}