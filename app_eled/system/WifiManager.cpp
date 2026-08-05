#include "headers.h"

WifiManager::WifiManager() {}

bool WifiManager::connect(const char* ssid, const char* password, unsigned long timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
        LOG_INFO("WiFi already connected");
        return true;
    }
    WiFi.begin(ssid, password);
    LOG_INFO("Connecting to WiFi SSID: %s", ssid);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeoutMs) {
            LOG_ERROR("WiFi connection timeout");
            return false;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    LOG_INFO("WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
    return true;
}

bool WifiManager::connectWithFallback(const WifiCredentials& newCreds,
                                       const char* fallbackSsid,
                                       const char* fallbackPassword,
                                       unsigned long timeoutMs) {
    LOG_INFO("Mencoba ganti WiFi ke SSID baru: %s", newCreds.ssid.c_str());

    disconnect();
    delay(100);

    if (connect(newCreds.ssid.c_str(), newCreds.password.c_str(), timeoutMs)) {
        LOG_INFO("Berhasil terhubung ke SSID baru: %s", newCreds.ssid.c_str());
        return true;
    }

    LOG_WARN("Gagal terhubung ke SSID baru (%s), kembali ke SSID awal: %s",
              newCreds.ssid.c_str(), fallbackSsid);

    disconnect();
    delay(100);

    if (connect(fallbackSsid, fallbackPassword, timeoutMs)) {
        LOG_INFO("Berhasil kembali terhubung ke SSID awal: %s", fallbackSsid);
    } else {
        LOG_ERROR("Gagal juga terhubung ke SSID awal: %s", fallbackSsid);
    }

    return false;
}

bool WifiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WifiManager::getLocalIP() const {
    return WiFi.localIP().toString();
}

int WifiManager::getRSSI() const {
    return WiFi.RSSI();
}

void WifiManager::disconnect() {
    WiFi.disconnect();
    LOG_INFO("WiFi disconnected");
}