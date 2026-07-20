#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WifiManager {
public:
    WifiManager();
    bool connect(const char* ssid, const char* password, unsigned long timeoutMs = 15000);
    bool isConnected() const;
    String getLocalIP() const;
    int getRSSI() const;
    void disconnect();
};