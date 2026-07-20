#pragma once
#include <Arduino.h>

struct WeatherData {
    uint32_t timestamp;          // Unix epoch
    float    temperature;        // °C
    uint8_t  humidity;           // %
    float    windSpeed;          // km/h
    float    windGust;           // km/h
    uint8_t  windDirection;      // indeks 0-15
    float    windDeg;            // derajat
    float    rainDelta;          // mm sejak terakhir
    float    rainTotal;          // mm akumulasi
    uint8_t  rainRaw;            // counter mentah
    float    light;              // lux
    uint8_t  sensorId;
    uint8_t  channel;
    bool     batteryOk;
};