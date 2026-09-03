#include "headers.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

CloudAPI::CloudAPI() {}

bool CloudAPI::begin(const char* serverURL, const char* apiKey) {
    this->server = String(serverURL);
    this->apiKey = String(apiKey);
    return true;
}

bool CloudAPI::uploadWeather(const WeatherData& data) {
    if (server.isEmpty()) {
        LOG_ERROR("No server URL, cannot upload");
        return false;
    }
    HTTPClient http;
    String url = server + ENDPOINT_SENSOR_POST;
    http.begin(url);
    http.setConnectTimeout(5000);
    http.setTimeout(8000);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(512);
    doc["datetime"]   = Helpers::epochToDateTimeStr(data.timestamp);
    doc["id"]         = DEVICE_ID;
    doc["ch"]         = data.channel;
    doc["batt"]       = data.batteryOk ? "OK" : "LOW";
    doc["temp_out"]   = data.temperature;
    doc["hum_out"]    = data.humidity;
    doc["wind_speed"] = data.windSpeed;
    doc["wind_gust"]  = data.windGust;
    doc["wind_dir"]   = Helpers::windDirToStr(data.windDirection);
    doc["wind_deg"]   = data.windDeg;
    doc["rain_delta"] = data.rainDelta;
    doc["rain_total"] = data.rainTotal;
    doc["rain_raw"]   = data.rainRaw;
    doc["light_lux"]  = data.light;

    String payload;
    serializeJson(doc, payload);
    LOG_DEBUG("POST %s payload=%s", url.c_str(), payload.c_str());

    int code = http.POST(payload);
    String response = http.getString();
    http.end();

    bool success = (code == 201);
    if (success) LOG_INFO("Weather uploaded via HTTP (%s)", response.c_str());
    else LOG_ERROR("HTTP upload failed, code %d, response=%s", code, response.c_str());
    return success;
}

bool CloudAPI::uploadStatus(const DeviceStatus& status) {
    // TODO: endpoint status belum ada di backend saat ini.
    (void)status;
    LOG_WARN("Status upload via HTTP not implemented");
    return false;
}

bool CloudAPI::fetchRemoteConfig(RemoteConfig& out) {
    if (server.isEmpty()) {
        LOG_ERROR("Server URL not set");
        return false;
    }

    HTTPClient http;
    String url = server + ENDPOINT_CONFIG_GET;
    http.begin(url);
    http.setConnectTimeout(5000);
    http.setTimeout(8000);

    int code = http.GET();
    if (code != 200) {
        LOG_WARN("GET %s failed, code %d", url.c_str(), code);
        http.end();
        return false;
    }

    String response = http.getString();
    http.end();

    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        LOG_ERROR("Config JSON parse error: %s", err.c_str());
        return false;
    }

    if (!doc.containsKey("configVersion")) {
        LOG_WARN("Response /config tidak punya field configVersion, diabaikan");
        return false;
    }
    out.configVersion = doc["configVersion"].as<uint32_t>();
    out.hasConfigVersion = true;

    if (doc.containsKey("wifiSSID"))     out.ssid = doc["wifiSSID"].as<String>();
    if (doc.containsKey("wifiPassword")) out.password = doc["wifiPassword"].as<String>();

    // "wifiInterval" adalah nama field baru (menggantikan konsep
    // "uploadInterval" dari arsitektur lama, karena sekarang WiFi-lah
    // yang di-duty-cycle, bukan siklus bangun/upload). Tetap terima
    // "uploadInterval" sebagai fallback supaya backend lama yang belum
    // sempat diupdate masih bisa dipakai.
    if (doc.containsKey("wifiInterval")) {
        out.wifiInterval = doc["wifiInterval"].as<uint32_t>();
    } else if (doc.containsKey("uploadInterval")) {
        out.wifiInterval = doc["uploadInterval"].as<uint32_t>();
    }

    LOG_INFO("Config diterima dari server: v%u", (unsigned)out.configVersion);
    return true;
}