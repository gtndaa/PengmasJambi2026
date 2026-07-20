#ifndef CONFIG_H
#define CONFIG_H

// ======================================================
// Device Configuration
// ======================================================

#define DEVICE_NAME "WeatherStation"

// ======================================================
// WiFi Configuration
// ======================================================

#define WIFI_TIMEOUT         20000      // ms
#define WIFI_RETRY_INTERVAL  5000       // ms

// ======================================================
// MQTT Configuration (Sprint 2)
// ======================================================

#define MQTT_PORT            8883
#define MQTT_KEEPALIVE       60

// MQTT Topics
#define TOPIC_DATA           "weather/data"
#define TOPIC_CONFIG         "weather/config"
#define TOPIC_STATUS         "weather/status"
#define TOPIC_HEARTBEAT      "weather/heartbeat"
#define TOPIC_ACK            "weather/ack"
#define TOPIC_COMMAND        "weather/cmd"

#endif