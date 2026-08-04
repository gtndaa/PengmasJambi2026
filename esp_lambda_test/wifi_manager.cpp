#include "wifi_manager.h"
#include "secrets.h"

void WifiManager::begin()
{
    Serial.println("--------------------------------");
    Serial.println("Connecting to WiFi...");
    Serial.println("--------------------------------");

    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("--------------------------------");
    Serial.println("WiFi Connected!");
    Serial.print("IP Address : ");
    Serial.println(WiFi.localIP());
    Serial.println("--------------------------------");
}

bool WifiManager::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

void WifiManager::reconnect()
{
    if (WiFi.status() == WL_CONNECTED)
        return;

    Serial.println("WiFi disconnected!");

    WiFi.disconnect();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi Reconnected!");
}

IPAddress WifiManager::localIP()
{
    return WiFi.localIP();
}