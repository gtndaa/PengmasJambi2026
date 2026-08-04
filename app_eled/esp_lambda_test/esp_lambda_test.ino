#include "config.h"
#include "wifi_manager.h"
#include "lambda_api.h"
#include "device_config.h"

WifiManager wifi;
LambdaAPI api;
DeviceConfig config;

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

    if(api.getDeviceConfig(config))
    {
        Serial.println();

        Serial.println("===== DEVICE CONFIG =====");

        Serial.print("SSID : ");

        Serial.println(config.wifiSSID);

        Serial.print("Password : ");

        Serial.println(config.wifiPassword);

        Serial.print("Upload Interval : ");

        Serial.println(config.uploadInterval);

        Serial.print("Listen Window : ");

        Serial.println(config.listenWindow);

        Serial.print("Timezone : ");

        Serial.println(config.timezoneOffset);

        Serial.print("Version : ");

        Serial.println(config.configVersion);

        Serial.print("Deep Sleep : ");

        Serial.println(config.useDeepSleep);

        Serial.println("=========================");
    }

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