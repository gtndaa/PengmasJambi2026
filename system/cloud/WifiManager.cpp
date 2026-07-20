#include "WiFiManager.h"

bool WiFiManager::connect(const char* ssid, const char* password, uint32_t timeoutMs) {
    WiFi.begin(ssid, password);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeoutMs) {
            connected = false;
            return false;
        }
        delay(100);
    }
    connected = true;
    return true;
}

void WiFiManager::disconnect() {
    WiFi.disconnect(true);
    connected = false;
}

bool WiFiManager::isConnected() const {
    return connected && (WiFi.status() == WL_CONNECTED);
}

int8_t WiFiManager::getRSSI() const {
    return WiFi.RSSI();
}

String WiFiManager::getLocalIP() const {
    return WiFi.localIP().toString();
}