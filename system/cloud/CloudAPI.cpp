#include "CloudAPI.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "utils/Logger.h"

CloudAPI* CloudAPI::instance = nullptr;

CloudAPI::CloudAPI(WiFiClient& client) : mqttClient(client), configCb(nullptr) {
    instance = this;
}

bool CloudAPI::begin(const char* serverURL,
                     const char* apiKey,
                     const char* mqttBroker,
                     uint16_t mqttPort,
                     const char* mqttClientId,
                     const char* mqttUsername,
                     const char* mqttPassword) {
    this->server = String(serverURL);
    this->apiKey = String(apiKey);
    this->mqttClientId = (mqttClientId ? String(mqttClientId) : "");
    
    if (mqttBroker != nullptr && strlen(mqttBroker) > 0) {
        mqttClient.setServer(mqttBroker, mqttPort);
        if (mqttUsername && strlen(mqttUsername) > 0) {
            mqttClient.setCredentials(mqttUsername, mqttPassword);
        }
        mqttClient.setCallback(mqttCallback);
        LOG_INFO("MQTT client initialized: %s:%d", mqttBroker, mqttPort);
    } else {
        LOG_INFO("MQTT not configured, using HTTP only");
    }
    return true;
}

bool CloudAPI::connectMQTT() {
    if (!mqttClient.connected()) {
        if (mqttClientId.isEmpty()) {
            LOG_WARN("MQTT clientId not set, cannot connect");
            return false;
        }
        bool ok = mqttClient.connect(mqttClientId.c_str());
        if (ok) {
            LOG_INFO("MQTT connected as %s", mqttClientId.c_str());
            // Subscribe ke topik konfigurasi perangkat
            String configTopic = "device/" + mqttClientId + "/config";
            subscribe(configTopic.c_str());
        } else {
            LOG_ERROR("MQTT connect failed, state=%d", mqttClient.state());
        }
        return ok;
    }
    return true;
}

bool CloudAPI::disconnectMQTT() {
    mqttClient.disconnect();
    return true;
}

bool CloudAPI::isMQTTConnected() const {
    return mqttClient.connected();
}

bool CloudAPI::uploadWeather(const WeatherData& data) {
    // Prioritas: MQTT jika tersambung
    if (mqttClient.connected() && !mqttClientId.isEmpty()) {
        DynamicJsonDocument doc(512);
        doc["deviceId"] = mqttClientId;
        doc["timestamp"] = data.timestamp;
        doc["temperature"] = data.temperature;
        doc["humidity"] = data.humidity;
        doc["windSpeed"] = data.windSpeed;
        doc["windGust"] = data.windGust;
        doc["windDirection"] = data.windDirection;
        doc["windDeg"] = data.windDeg;
        doc["rainDelta"] = data.rainDelta;
        doc["rainTotal"] = data.rainTotal;
        doc["rainRaw"] = data.rainRaw;
        doc["light"] = data.light;
        doc["sensorId"] = data.sensorId;
        doc["channel"] = data.channel;
        doc["batteryOk"] = data.batteryOk;

        String payload;
        serializeJson(doc, payload);
        String topic = "device/" + mqttClientId + "/weather";
        bool success = mqttClient.publish(topic.c_str(), payload.c_str());
        if (success) {
            LOG_DEBUG("Weather published via MQTT to %s", topic.c_str());
        } else {
            LOG_ERROR("Failed to publish weather via MQTT");
        }
        return success;
    }
    
    // Fallback: HTTP (jika server tidak kosong)
    if (server.isEmpty()) {
        LOG_ERROR("No server URL, cannot upload");
        return false;
    }
    HTTPClient http;
    http.begin(server + "/weather");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", apiKey);
    
    DynamicJsonDocument doc(512);
    doc["device"] = mqttClientId; // atau device ID lain
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
    bool success = (code == 200 || code == 201);
    if (success) LOG_INFO("Weather uploaded via HTTP");
    else LOG_ERROR("HTTP upload failed, code %d", code);
    return success;
}

bool CloudAPI::uploadStatus(const DeviceStatus& status) {
    if (mqttClient.connected() && !mqttClientId.isEmpty()) {
        DynamicJsonDocument doc(256);
        doc["deviceId"] = mqttClientId;
        doc["batteryVoltage"] = status.batteryVoltage;
        doc["superCapVoltage"] = status.superCapVoltage;
        doc["wifiRSSI"] = status.wifiRSSI;
        doc["freeHeap"] = status.freeHeap;
        doc["uptime"] = status.uptime;
        doc["bootCount"] = status.bootCount;
        doc["wakeCounter"] = status.wakeCounter;
        doc["firmwareVersion"] = status.firmwareVersion;
        doc["sdCardPresent"] = status.sdCardPresent;
        doc["rtcPresent"] = status.rtcPresent;
        doc["lightSensorPresent"] = status.lightSensorPresent;
        doc["radioPresent"] = status.radioPresent;

        String payload;
        serializeJson(doc, payload);
        String topic = "device/" + mqttClientId + "/status";
        return mqttClient.publish(topic.c_str(), payload.c_str());
    }
    // Fallback HTTP jika diperlukan
    LOG_WARN("Status upload via HTTP not implemented");
    return false;
}

bool CloudAPI::getConfig(String& configJSON) {
    if (server.isEmpty()) {
        LOG_ERROR("Server URL not set");
        return false;
    }
    HTTPClient http;
    http.begin(server + "/config");
    http.addHeader("X-API-Key", apiKey);
    int code = http.GET();
    if (code == 200) {
        configJSON = http.getString();
        LOG_INFO("Config retrieved via HTTP");
        http.end();
        return true;
    } else {
        LOG_ERROR("HTTP getConfig failed, code %d", code);
        http.end();
        return false;
    }
}

bool CloudAPI::subscribe(const char* topic) {
    if (!mqttClient.connected()) {
        LOG_WARN("MQTT not connected, cannot subscribe");
        return false;
    }
    bool ok = mqttClient.subscribe(topic);
    if (ok) LOG_INFO("Subscribed to %s", topic);
    else LOG_ERROR("Subscribe to %s failed", topic);
    return ok;
}

void CloudAPI::setConfigCallback(ConfigCallback cb) {
    configCb = cb;
}

void CloudAPI::loop() {
    if (mqttClient.connected()) {
        mqttClient.loop();
    }
}

// ----- static callback -----
void CloudAPI::mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (instance) {
        instance->handleMessage(topic, payload, length);
    }
}

void CloudAPI::handleMessage(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
    LOG_DEBUG("MQTT msg on %s: %s", topic, message.c_str());

    String configTopic = "device/" + mqttClientId + "/config";
    if (String(topic) == configTopic && configCb) {
        configCb(message);
    }
}