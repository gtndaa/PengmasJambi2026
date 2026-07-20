#include "WifiManager.h"
#include "utils/Logger.h"

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