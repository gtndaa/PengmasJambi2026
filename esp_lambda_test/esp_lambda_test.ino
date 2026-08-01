#include "config.h"
#include "wifi_manager.h"
#include "lambda_api.h"

WifiManager wifi;
LambdaAPI api;

unsigned long lastPrint = 0;
unsigned long lastGet = 0;

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("==============================");
    Serial.println(DEVICE_NAME);
    Serial.println("==============================");

    wifi.begin();

    api.begin();

    delay(1000);

    api.postSensorData();
}

void loop()
{
    wifi.reconnect();

    if (millis() - lastGet > 10000)
{
    lastGet = millis();

    api.getLatestSensor();
}

    if (millis() - lastPrint >= 5000)
    {
        lastPrint = millis();

        Serial.println("--------------------------");

        if (wifi.isConnected())
        {
            Serial.println("WiFi Status : CONNECTED");
            Serial.print("IP Address  : ");
            Serial.println(wifi.localIP());
        }
        else
        {
            Serial.println("WiFi Status : DISCONNECTED");
        }

        Serial.println("--------------------------");
    }
}