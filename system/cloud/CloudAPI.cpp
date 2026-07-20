#include "CloudAPI.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "utils/Logger.h"

bool CloudAPI::begin(const char* serverURL, const char* apiKey) {
    server = String(serverURL);
    key = String(apiKey);
    return true;
}

bool CloudAPI::uploadWeather(const WeatherData& data) {
    if (server.isEmpty()) return false;
    HTTPClient http;
    http.begin(server + "/weather");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", key);

    DynamicJsonDocument doc(512);
    doc["device"] = DEVICE_ID;
    doc["timestamp"] = data.timestamp;
    doc["temp"] = data.temperature;
    doc["hum"] = data.humidity;
    doc["wind_speed"] = data.windSpeed;
    doc["wind_gust"] = data.windGust;
    doc["wind_dir"] = data.windDirection;
    doc["wind_deg"] = data.windDeg;
    doc["rain_delta"] = data.rainDelta;
    doc["rain_total"] = data.rainTotal;
    doc["light"] = data.light;
    doc["batt_ok"] = data.batteryOk;
    doc["channel"] = data.channel;

    String payload;
    serializeJson(doc, payload);

    int code = http.POST(payload);
    http.end();
    if (code == 200 || code == 201) {
        LOG_INFO("Upload weather OK");
        return true;
    } else {
        LOG_ERROR("Upload weather failed, HTTP %d", code);
        return false;
    }
}

bool CloudAPI::uploadStatus(const DeviceStatus& status) {
    // Implementasi serupa
    return true;
}

bool CloudAPI::getConfig(String& configJSON) {
    // Stub: kembalikan konfigurasi default
    configJSON = "{\"uploadInterval\":60000,\"listenWindow\":30000}";
    return true;
}