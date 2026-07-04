#include "config.h"

volatile uint32_t pulseBuf[MAX_PULSES];
volatile uint16_t pulseIdx    = 0;
volatile uint32_t lastPulseMs = 0;
volatile bool     gotPacket   = false;

BH1750     lightMeter;
float      luxValue    = 0.0f;

RTC_DS3231 rtc;
bool       rtcOK       = false;

float      rainAccumulated = 0.0f;

static volatile uint32_t risingTs       = 0;
static uint32_t          lastLuxRead    = 0;
static uint8_t           rainCounterPrev = RAIN_UNINIT;

static const char *DAYS[] = {
  "Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu"
};

// CC1101 for WH5300
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

void initCC1101() {
  ELECHOUSE_cc1101.setSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CSN);
  ELECHOUSE_cc1101.setGDO(GDO0_PIN, GDO2_PIN);
  ELECHOUSE_cc1101.Init();
  if (!ELECHOUSE_cc1101.getCC1101()) {
    Serial.println(F("[ERROR] CC1101 tidak terdeteksi! Cek wiring."));
    while (true) delay(1000);
  }
  ELECHOUSE_cc1101.setMHZ(RF_FREQ_MHZ);
  ELECHOUSE_cc1101.setModulation(RF_MODULATION);
  ELECHOUSE_cc1101.setDRate(RF_DATARATE);
  ELECHOUSE_cc1101.setRxBW(RF_RXBW);
  ELECHOUSE_cc1101.SetRx();
  Serial.println(F("[OK] CC1101 RX OOK 433.92 MHz"));
}

// RTC DS3231
void initRTC() {
  if (!rtc.begin()) {
    Serial.println(F("[WARN] DS3231 tidak terdeteksi! Cek wiring I2C."));
    return;
  }
  rtcOK = true;
  if (rtc.lostPower()) {
    // Untuk set manual: rtc.adjust(DateTime(YYYY, M, D, H, Min, S));
    Serial.println(F("[WARN] RTC kehilangan daya, set ke waktu kompilasi"));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  Serial.printf("[OK] DS3231 — %s\n", getDateTimeStr().c_str());
  Serial.printf("[OK] Suhu RTC : %.2f °C\n", rtc.getTemperature());
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

// BH1750 Light Intensity
void initLux() {
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("[OK] BH1750 terdeteksi (0x23)"));
  } else {
    Serial.println(F("[WARN] BH1750 tidak terdeteksi! Cek wiring I2C."));
  }
}

void updateLux() {
  if (millis() - lastLuxRead < LUX_INTERVAL) return;
  lastLuxRead = millis();
  float lux = lightMeter.readLightLevel();
  if (lux >= 0) luxValue = lux;
}

// Rain calculation
float calcRainDelta(uint8_t currentCounter) {
  if (rainCounterPrev == RAIN_UNINIT) {
    rainCounterPrev = currentCounter;
    return 0.0f;
  }
  uint8_t prev = rainCounterPrev;
  rainCounterPrev = currentCounter;
  if (currentCounter == prev) return 0.0f;

  uint8_t delta = (currentCounter > prev)
    ? currentCounter - prev
    : (uint8_t)(256 - prev + currentCounter);

  float mm = delta * RAIN_MM_PER_TIP;
  rainAccumulated += mm;
  return mm;
}

// Packet CRC-8 (polynomial 0x31, initial value 0x00)
uint8_t crc8(uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
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
