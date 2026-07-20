#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
    bool connect(const char* ssid, const char* password, uint32_t timeoutMs = 10000);
    void disconnect();
    bool isConnected() const;
    int8_t getRSSI() const;
    String getLocalIP() const;
private:
    bool connected = false;
};