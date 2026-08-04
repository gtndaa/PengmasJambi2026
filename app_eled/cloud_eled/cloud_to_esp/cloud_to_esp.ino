#include "config.h"
#include "secrets.h"
#include "wifi_manager.h"

WifiManager wifi;

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("==============================");
    Serial.println(DEVICE_NAME);
    Serial.println("==============================");

    wifi.begin();
}

void loop()
{
    wifi.loop();

    if (wifi.isConnected())
    {
        // Sprint 2
        // MQTT.loop();
    }

    delay(100);
}