#include "lambda_api.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "config.h"

bool LambdaAPI::begin()
{
    HTTPClient http;

    String url = String(BASE_URL) + "/";

    Serial.println();
    Serial.println("=================================");
    Serial.println("Connecting to Lambda...");
    Serial.println(url);
    Serial.println("=================================");

    http.begin(url);

    int httpCode = http.GET();

    Serial.print("HTTP Code : ");
    Serial.println(httpCode);

    if (httpCode > 0)
    {
        String payload = http.getString();

        Serial.println("Response:");

        Serial.println(payload);

        http.end();

        return true;
    }

    Serial.println("GET Failed");

    http.end();

    return false;
}

bool LambdaAPI::getLatestSensor()
{
    HTTPClient http;

    String url = String(BASE_URL) + "/sensordata";

    Serial.println();
    Serial.println("==============================");
    Serial.println("GET Latest Sensor");
    Serial.println(url);
    Serial.println("==============================");

    http.begin(url);

    int httpCode = http.GET();

    Serial.print("HTTP Code : ");
    Serial.println(httpCode);

    if (httpCode > 0)
    {
        String response = http.getString();

        Serial.println("Response:");

        Serial.println(response);

        http.end();

        return true;
    }

    Serial.println("GET Failed");

    http.end();

    return false;
}

bool LambdaAPI::postSensorData()
{
    HTTPClient http;

    String url = String(BASE_URL) + "/sensordata";

    Serial.println();
    Serial.println("==============================");
    Serial.println("POST Sensor");
    Serial.println(url);
    Serial.println("==============================");

    http.begin(url);

    http.addHeader("Content-Type", "application/json");

    String json =
    "{"
        "\"datetime\":\"2026-08-01 16:30:00\","
        "\"id\":\"ESP_TEST\","
        "\"ch\":1,"
        "\"batt\":\"OK\","
        "\"temp_out\":29.5,"
        "\"hum_out\":82,"
        "\"wind_speed\":2.1,"
        "\"wind_gust\":3.8,"
        "\"wind_dir\":\"NW\","
        "\"wind_deg\":315,"
        "\"rain_delta\":0,"
        "\"rain_total\":10.5,"
        "\"rain_raw\":15,"
        "\"light_lux\":8500"
        "}";    

    Serial.println("Payload:");

    Serial.println(json);

    int httpCode = http.POST(json);

    Serial.print("HTTP Code : ");

    Serial.println(httpCode);

    String response = http.getString();

    Serial.println("Response:");

    Serial.println(response);

    http.end();

    return (httpCode == 201);
}

bool LambdaAPI::getDeviceConfig(DeviceConfig &config)
{
    HTTPClient http;

    String url = String(BASE_URL) + "/device-config";

    Serial.println();
    Serial.println("==============================");
    Serial.println("GET Device Config");
    Serial.println(url);
    Serial.println("==============================");

    http.begin(url);

    int httpCode = http.GET();

    if (httpCode != 200)
    {
        Serial.print("HTTP Error : ");
        Serial.println(httpCode);

        http.end();

        return false;
    }

    String payload = http.getString();

    Serial.println(payload);

    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, payload);

    if (err)
    {
        Serial.println("JSON Parse Error");

        http.end();

        return false;
    }

    config.wifiSSID = doc["wifiSSID"].as<String>();

    config.wifiPassword = doc["wifiPassword"].as<String>();

    config.uploadInterval = doc["uploadInterval"];

    config.listenWindow = doc["listenWindow"];

    config.timezoneOffset = doc["timezoneOffset"];

    config.configVersion = doc["configVersion"];

    config.useDeepSleep = doc["useDeepSleep"];

    http.end();

    return true;
}