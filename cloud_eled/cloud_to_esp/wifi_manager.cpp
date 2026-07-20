#include "wifi_manager.h"
#include "config.h"
#include "secrets.h"

void WifiManager::begin()
{
    Serial.println();
    Serial.println("--------------------------------");
    Serial.println("Connecting to WiFi...");
    Serial.println("--------------------------------");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");

        delay(500);

        if (millis() - startTime > WIFI_TIMEOUT)
        {
            Serial.println();
            Serial.println("WiFi connection timeout");
            return;
        }
    }

    Serial.println();
    Serial.println("WiFi Connected");
    Serial.print("IP Address : ");
    Serial.println(WiFi.localIP());
}

void WifiManager::reconnect()
{
    Serial.println("Reconnecting WiFi...");

    WiFi.disconnect();

    delay(1000);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void WifiManager::loop()
{
    if (WiFi.status() == WL_CONNECTED)
        return;

    if (millis() - previousReconnect < WIFI_RETRY_INTERVAL)
        return;

    previousReconnect = millis();

    reconnect();
}

bool WifiManager::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

String WifiManager::getIP()
{
    return WiFi.localIP().toString();
}