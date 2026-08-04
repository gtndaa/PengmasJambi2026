#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <BH1750.h>
#include <RTClib.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

// ─── Project info ─────────────────────────────────────────────
#define PROJECT_NAME    "WEATHER STATION ELED"
#define FW_VERSION      "1.0.0"

// CC1101 configuration
#define GDO2_PIN        4
#define GDO0_PIN        2
#define CC1101_SCK      18
#define CC1101_MISO     19
#define CC1101_MOSI     23
#define CC1101_CSN      5

// I2C configuration
#define I2C_SDA         21
#define I2C_SCL         22

// RF configuration
#define RF_FREQ_MHZ     433.92f
#define RF_MODULATION   2          // ASK/OOK
#define RF_DATARATE     4.8f
#define RF_RXBW         203.12f

#define PULSE_1_MIN     350
#define PULSE_1_MAX     650
#define PULSE_0_MIN     1100
#define PULSE_0_MAX     1590
#define PACKET_TIMEOUT  30         // ms tanpa pulsa baru → commit paket

#define MAX_PULSES      300
#define MAX_BYTES       12

// BH1750 configuration
#define LUX_INTERVAL    2000       // ms antar pembacaan lux

// WH5300 configuration
#define RAIN_UNINIT     0xFF
#define RAIN_MM_PER_TIP 0.3f
#define WIND_FACTOR     1.216f
#define TEMP_OFFSET     400
#define TEMP_DIVISOR    10.0f
#define WIND_DEG_STEP   22.5f

extern volatile uint32_t pulseBuf[MAX_PULSES];
extern volatile uint16_t pulseIdx;
extern volatile uint32_t lastPulseMs;
extern volatile bool     gotPacket;

extern BH1750      lightMeter;
extern float       luxValue;
extern RTC_DS3231  rtc;
extern bool        rtcOK;
extern float       rainAccumulated;

void        initCC1101();
void        initRTC();
void        initLux();
void IRAM_ATTR gdo2ISR();
void        updateLux();
float       calcRainDelta(uint8_t currentCounter);
uint8_t     crc8(uint8_t *data, uint8_t len);
uint16_t    pulsesToBits(uint32_t *pulses, uint16_t count, uint8_t *bits, uint16_t maxBits);
bool        scanForPacket(uint8_t *bits, uint16_t bitCount, uint8_t *out, uint8_t *outLen);
String      getDateTimeStr();
String      getISOStr();

void        printWeather(uint8_t *packet, uint8_t len);