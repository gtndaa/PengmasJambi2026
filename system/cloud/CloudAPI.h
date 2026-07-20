#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "models/WeatherData.h"
#include "models/DeviceStatus.h"
#include "utils/Logger.h"

// Callback untuk menerima konfigurasi via MQTT (opsional)
typedef std::function<void(const String& configJSON)> ConfigCallback;

class CloudAPI {
public:
    CloudAPI(WiFiClient& client);
    
    // Inisialisasi: serverURL & apiKey untuk HTTP getConfig,
    // plus parameter MQTT (jika nullptr, MQTT tidak digunakan)
    bool begin(const char* serverURL,
               const char* apiKey,
               const char* mqttBroker = nullptr,
               uint16_t mqttPort = 1883,
               const char* mqttClientId = nullptr,
               const char* mqttUsername = nullptr,
               const char* mqttPassword = nullptr);
    
    // Koneksi MQTT
    bool connectMQTT();
    bool disconnectMQTT();
    bool isMQTTConnected() const;
    
    // Upload via MQTT (jika aktif) atau fallback ke HTTP (jika perlu)
    bool uploadWeather(const WeatherData& data);
    bool uploadStatus(const DeviceStatus& status);
    
    // Dapatkan konfigurasi dari server via HTTP GET
    bool getConfig(String& configJSON);
    
    // Subscribe ke topik MQTT (misal untuk konfigurasi)
    bool subscribe(const char* topic);
    void setConfigCallback(ConfigCallback cb);
    
    // Harus dipanggil di loop()
    void loop();

private:
    // HTTP
    String server;
    String apiKey;
    
    // MQTT
    PubSubClient mqttClient;
    String mqttClientId;
    ConfigCallback configCb;
    
    static CloudAPI* instance;
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    void handleMessage(char* topic, byte* payload, unsigned int length);
};