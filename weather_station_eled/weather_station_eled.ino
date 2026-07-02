/*
 * ============================================================
 *  Weather Station ELED
 *  CC1101 WH5300 Outdoor Sensor Receiver + BH1750FVI Light Sensor
 *
 *  BH1750FVI Wiring (I2C addr 0x23)):
 *    VCC → 3.3V, GND → GND
 *    SDA → GPIO 21, SCL → GPIO 22
 *    ADDR → GND
 *
 *  DS3231 Wiring (I2C addr 0x68):
 *    VCC → 3.3V, GND → GND
 *    SDA → GPIO 21, SCL → GPIO 22
 *
 *  CC1101 Wiring:
 *    VCC → 3.3V, GND → GND
 *    SCK=18, MISO=19, MOSI=23, CSN=5, GDO0=2, GDO2=4
 * ============================================================
 */

#include <SPI.h>
#include <Wire.h>
#include <BH1750.h>
#include <RTClib.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define GDO2_PIN  4
#define GDO0_PIN  2

BH1750   lightMeter;
float    luxValue     = 0.0f;
uint32_t lastLuxRead  = 0;
#define  LUX_INTERVAL 2000

RTC_DS3231 rtc;
bool rtcOK = false;
const char *DAYS[] = {
  "Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu"
};

// Tuned timing pulse in µs
#define PULSE_1_MIN    350
#define PULSE_1_MAX    650
#define PULSE_0_MIN   1100
#define PULSE_0_MAX   1590
#define PACKET_TIMEOUT  30

#define MAX_PULSES  300
#define MAX_BYTES    12

volatile uint32_t pulseBuf[MAX_PULSES];
volatile uint16_t pulseIdx    = 0;
volatile uint32_t lastPulseMs = 0;
volatile bool     gotPacket   = false;
volatile uint32_t risingTs    = 0;

#define RAIN_UNINIT    0xFF
#define RAIN_MM_PER_TIP 0.3f

uint8_t  rainCounterPrev  = RAIN_UNINIT;  // previous packet tip counter
float    rainAccumulated  = 0.0f;         // accumulated rain since boot (mm)
uint32_t rainLastResetMs  = 0;

// ISR
void IRAM_ATTR gdo2ISR() {
  uint32_t now = micros();
  if (digitalRead(GDO2_PIN) == HIGH) {
    risingTs = now;
    return;
  }
  uint32_t dur = now - risingTs;
  if (gotPacket) return;
  if ((dur >= PULSE_1_MIN && dur <= PULSE_1_MAX) ||
      (dur >= PULSE_0_MIN && dur <= PULSE_0_MAX)) {
    if (pulseIdx < MAX_PULSES) {
      pulseBuf[pulseIdx++] = dur;
      lastPulseMs = millis();
    }
  }
}

// CRC-8 poly 0x31 for wind direction
uint8_t crc8(uint8_t *d, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= d[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
  }
  return crc;
}

uint16_t pulsesToBits(uint32_t *pulses, uint16_t count,
                      uint8_t *bits, uint16_t maxBits) {
  uint16_t n = 0;
  for (uint16_t i = 0; i < count && n < maxBits; i++) {
    if      (pulses[i] >= PULSE_1_MIN && pulses[i] <= PULSE_1_MAX) bits[n++] = 1;
    else if (pulses[i] >= PULSE_0_MIN && pulses[i] <= PULSE_0_MAX) bits[n++] = 0;
  }
  return n;
}

bool scanForPacket(uint8_t *bits, uint16_t bitCount,
                   uint8_t *out, uint8_t *outLen) {
  for (uint16_t offset = 0; offset + 64 <= bitCount; offset++) {
    for (uint8_t nbytes = 8; nbytes <= MAX_BYTES; nbytes++) {
      if (offset + (uint16_t)nbytes * 8 > bitCount) break;
      uint8_t candidate[MAX_BYTES] = {0};
      for (uint8_t b = 0; b < nbytes; b++) {
        uint8_t byte = 0;
        for (uint8_t bit = 0; bit < 8; bit++)
          byte = (byte << 1) | bits[offset + b * 8 + bit];
        candidate[b] = byte;
      }
      if (crc8(candidate, nbytes - 1) == candidate[nbytes - 1] &&
          candidate[0] == 0xAA) {
        memcpy(out, candidate, nbytes);
        *outLen = nbytes;
        return true;
      }
    }
  }
  return false;
}

// Rain Handler (wrap-around)
float calcRainDelta(uint8_t currentCounter) {
  if (rainCounterPrev == RAIN_UNINIT) {
    rainCounterPrev = currentCounter;
    return 0.0f;
  }

  uint8_t prev = rainCounterPrev;
  rainCounterPrev = currentCounter;

  if (currentCounter == prev) {
    return 0.0f;
  }

  uint8_t delta;
  if (currentCounter > prev) {
    delta = currentCounter - prev;
  } else {
    // Wrap if counter passes 255 → 0
    delta = (uint8_t)(256 - prev + currentCounter);
  }

  float mm = delta * RAIN_MM_PER_TIP;
  rainAccumulated += mm;
  return mm;
}

void updateLux() {
  if (millis() - lastLuxRead >= LUX_INTERVAL) {
    lastLuxRead = millis();
    float lux = lightMeter.readLightLevel();
    if (lux >= 0) luxValue = lux;
  }
}

String getDateTimeStr() {
  if (!rtcOK) return String("RTC ERROR");
  DateTime now = rtc.now();
  char buf[40];
  snprintf(buf, sizeof(buf), "%s, %02d/%02d/%04d %02d:%02d:%02d",
    DAYS[now.dayOfTheWeek()],
    now.day(), now.month(), now.year(),
    now.hour(), now.minute(), now.second()
  );
  return String(buf);
}

String getISOStr() {
  if (!rtcOK) return String("1970-01-01T00:00:00");
  DateTime now = rtc.now();
  char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
    now.year(), now.month(), now.day(),
    now.hour(), now.minute(), now.second()
  );
  return String(buf);
}

void printWeather(uint8_t *d, uint8_t len) {
  if (len < 10) {
    Serial.println(F("[WARN] Paket terlalu pendek"));
    return;
  }

  const char *dirs[] = {
    "N","NNE","NE","ENE","E","ESE","SE","SSE",
    "S","SSW","SW","WSW","W","WNW","NW","NNW"
  };

  // Decode field WH5300
  uint8_t  sensorID  = d[1];
  uint16_t rawT      = ((uint16_t)(d[1] & 0x0F) << 8) | d[2];
  float    tempOut   = (rawT - 400) / 10.0f;
  uint8_t  humOut    = d[3];
  float    windSpeed = d[4] * 1.216f;
  float    windGust  = d[5] * 1.216f;
  uint8_t  rainRaw   = d[6];
  bool     battOK    = (d[7] & 0x80) != 0;
  uint8_t  channel   = (d[7] >> 4) & 0x07;
  uint8_t  windDirI  = d[8] & 0x0F;
  float    windDeg   = windDirI * 22.5f;

  float rainDelta = calcRainDelta(rainRaw);

  String dtStr  = getDateTimeStr();
  String isoStr = getISOStr();

  Serial.println(F("\n╔══════════════════════════════════════╗"));
  Serial.println(F(  "║        WEATHER STATION ELED          ║"));
  Serial.println(F(  "╚══════════════════════════════════════╝"));
  Serial.printf(     "  Waktu      : %s\n",         dtStr.c_str());
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Sensor ID  : 0x%02X  Ch: %d  Batt: %s\n",
                     sensorID, channel, battOK ? "OK" : "LOW ⚠");
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Suhu OUT   : %.1f °C\n",    tempOut);
  Serial.printf(     "  Humidity   : %d %%\n",       humOut);
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Angin      : %.1f km/h\n",   windSpeed);
  Serial.printf(     "  Gust       : %.1f km/h\n",   windGust);
  Serial.printf(     "  Arah       : %s (%.1f°)\n",  dirs[windDirI], windDeg);
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Rain delta : %.1f mm\n",     rainDelta);
  Serial.printf(     "  Rain total : %.1f mm\n",     rainAccumulated);
  Serial.printf(     "  Rain raw   : %d tips\n",     rainRaw);
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Light      : %.1f lux\n",    luxValue);
  Serial.println(F(  "══════════════════════════════════════"));

  // JSON
  Serial.println(F("[JSON]"));
  Serial.print(F("{"));
  Serial.printf("\"datetime\":\"%s\",",    isoStr.c_str());
  Serial.printf("\"id\":\"0x%02X\",",      sensorID);
  Serial.printf("\"ch\":%d,",              channel);
  Serial.printf("\"batt\":\"%s\",",        battOK ? "OK" : "LOW");
  Serial.printf("\"temp_out\":%.1f,",      tempOut);
  Serial.printf("\"hum_out\":%d,",         humOut);
  Serial.printf("\"wind_speed\":%.1f,",    windSpeed);
  Serial.printf("\"wind_gust\":%.1f,",     windGust);
  Serial.printf("\"wind_dir\":\"%s\",",    dirs[windDirI]);
  Serial.printf("\"wind_deg\":%.1f,",      windDeg);
  Serial.printf("\"rain_delta\":%.1f,",    rainDelta);
  Serial.printf("\"rain_total\":%.1f,",    rainAccumulated);
  Serial.printf("\"rain_raw\":%d,",        rainRaw);
  Serial.printf("\"light_lux\":%.1f",      luxValue);
  Serial.println(F("}"));
}

void initCC1101() {
  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, 5);
  ELECHOUSE_cc1101.setGDO(GDO0_PIN, GDO2_PIN);
  ELECHOUSE_cc1101.Init();

  if (!ELECHOUSE_cc1101.getCC1101()) {
    Serial.println(F("[ERROR] CC1101 tidak terdeteksi! Cek wiring."));
    while (true) delay(1000);
  }

  ELECHOUSE_cc1101.setMHZ(433.92);
  ELECHOUSE_cc1101.setModulation(2);
  ELECHOUSE_cc1101.setDRate(4.8);
  ELECHOUSE_cc1101.setRxBW(203.12);
  ELECHOUSE_cc1101.SetRx();
}

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println(F("\n══════════════════════════════════════"));
  Serial.println(F(  "   WEATHER STATION ELED STARTING..."));
  Serial.println(F(  "══════════════════════════════════════"));

  // Init I2C
  Wire.begin(21, 22);

  // Init RTC
  if (rtc.begin()) {
    rtcOK = true;
    if (rtc.lostPower()) {
      // Manual adjust if needed:rtc.adjust(DateTime(2026, 7, 2, 14, 30, 0));
      Serial.println(F("[WARN] RTC kehilangan daya, set ke waktu kompilasi"));
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    DateTime now = rtc.now();
    Serial.printf("[OK] DS3231 — %s\n", getDateTimeStr().c_str());
    Serial.printf("[OK] Suhu RTC : %.2f °C\n", rtc.getTemperature());
  } else {
    Serial.println(F("[WARN] DS3231 tidak terdeteksi! Cek wiring I2C."));
  }

  // Init BH1750
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("[OK] BH1750 terdeteksi (0x23)"));
  } else {
    Serial.println(F("[WARN] BH1750 tidak terdeteksi! Cek wiring I2C."));
  }

  // Init CC1101
  initCC1101();
  Serial.println(F("[OK] CC1101 RX OOK 433.92 MHz"));

  pinMode(GDO2_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(GDO2_PIN), gdo2ISR, CHANGE);
  Serial.println(F("[OK] Interrupt aktif di GPIO 4"));
  Serial.println(F("[OK] Rain counter: delta mode (wrap-around handled)"));
  Serial.println(F("[OK] Menunggu paket WH5300... (~48 detik antar TX)\n"));
}

// main program
void loop() {
  uint32_t now = millis();

  updateLux();

  // Commit via timeout
  if (!gotPacket && pulseIdx >= 64) {
    noInterrupts();
    uint32_t lastMs = lastPulseMs;
    interrupts();
    if (now - lastMs >= PACKET_TIMEOUT) gotPacket = true;
  }

  // clean noise
  if (!gotPacket && pulseIdx > 0 && pulseIdx < 64) {
    noInterrupts();
    uint32_t lastMs = lastPulseMs;
    interrupts();
    if (now - lastMs >= PACKET_TIMEOUT * 3) {
      noInterrupts();
      pulseIdx = 0;
      interrupts();
    }
  }

  // Watchdog: reinit CC1101 every minute
  static uint32_t lastReinit = 0;
  if (now - lastReinit > 60000) {
    lastReinit = now;
    ELECHOUSE_cc1101.SetRx();
  }

  if (!gotPacket) return;

  // get atomic buffer
  noInterrupts();
  uint16_t count = pulseIdx;
  uint32_t local[MAX_PULSES];
  memcpy(local, (const void*)pulseBuf, count * sizeof(uint32_t));
  pulseIdx  = 0;
  gotPacket = false;
  interrupts();

  // Convert pulses to bit
  uint8_t bits[MAX_PULSES] = {0};
  uint16_t bitCount = pulsesToBits(local, count, bits, MAX_PULSES);
  if (bitCount < 64) return;

  // Look for valid packets
  uint8_t packet[MAX_BYTES] = {0};
  uint8_t pktLen = 0;
  if (!scanForPacket(bits, bitCount, packet, &pktLen)) return;

  // Print RAW
  Serial.print(F("[RAW] "));
  for (int i = 0; i < pktLen; i++) Serial.printf("%02X ", packet[i]);
  Serial.println();

  // Print data
  printWeather(packet, pktLen);
}
