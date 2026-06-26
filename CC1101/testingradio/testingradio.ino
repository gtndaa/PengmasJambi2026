/**
 * ============================================================
 *  ESP32 + CC1101 — WH5300 Outdoor Sensor Receiver (FINAL v3)
 *
 *  Format paket 10-byte (dikalibrasi dari data nyata):
 *
 *  [0]  0xAA  → Sync marker (konstan)
 *  [1]  0x22  → Sensor ID
 *  [2]        → Suhu OUT high byte
 *               Temp = (((B1&0x0F)<<8 | B2) - 400) / 10.0 °C
 *  [3]        → Humidity OUT (%)
 *  [4]        → Wind speed raw  → Speed = raw × 1.21 km/h
 *  [5]        → Wind gust raw   → Gust  = raw × 1.21 km/h
 *  [6]        → Rain tip count  → Rain  = raw × 0.3 mm
 *  [7]        → [bit7]=batt OK, [6:4]=channel, [3:0]=flags
 *  [8]        → Wind direction index (0–15, 0=N, CW, 22.5°/step)
 *  [9]        → CRC-8 (poly 0x31)
 *
 *  Catatan arah angin:
 *    Display WH5300 menampilkan 8 arah utama (rounded), sedangkan
 *    protokol RF encode 16 arah (presisi 22.5°). Contoh:
 *    byte[8]=5 → ESE (display tampilkan "E")
 *    byte[8]=11 → WSW (display tampilkan "SW")
 *    Kode ini menampilkan 16 arah presisi penuh.
 *
 *  Data INDOOR (Tin/Hin) tidak ada di paket RF outdoor ini.
 *
 *  Wiring ESP32 ↔ CC1101:
 *    VCC→3.3V, GND→GND
 *    SCK=18, MISO=19, MOSI=23, CSN=5, GDO0=2, GDO2=4
 * ============================================================
 */

#include <SPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

// ─── Pin ──────────────────────────────────────────────────────
#define GDO2_PIN  4
#define GDO0_PIN  2

// ─── Timing pulse µs (dikalibrasi dari data nyata) ────────────
#define PULSE_1_MIN    350    // bit '1': ~449–510µs
#define PULSE_1_MAX    650
#define PULSE_0_MIN   1100    // bit '0': ~1443–1493µs
#define PULSE_0_MAX   1590
#define PACKET_TIMEOUT  30    // ms tanpa pulsa baru → commit paket

// ─── Buffer ───────────────────────────────────────────────────
#define MAX_PULSES   300
#define MAX_BYTES     12

volatile uint32_t pulseBuf[MAX_PULSES];
volatile uint16_t pulseIdx    = 0;
volatile uint32_t lastPulseMs = 0;
volatile bool     gotPacket   = false;
volatile uint32_t risingTs    = 0;

// ─── ISR ──────────────────────────────────────────────────────
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

// ─── CRC-8 poly 0x31 ─────────────────────────────────────────
uint8_t crc8(uint8_t *d, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= d[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
  }
  return crc;
}

// ─── Pulsa → bitstream ────────────────────────────────────────
uint16_t pulsesToBits(uint32_t *pulses, uint16_t count,
                      uint8_t *bits, uint16_t maxBits) {
  uint16_t n = 0;
  for (uint16_t i = 0; i < count && n < maxBits; i++) {
    if      (pulses[i] >= PULSE_1_MIN && pulses[i] <= PULSE_1_MAX) bits[n++] = 1;
    else if (pulses[i] >= PULSE_0_MIN && pulses[i] <= PULSE_0_MAX) bits[n++] = 0;
  }
  return n;
}

// ─── Scan bitstream → cari paket CRC valid ────────────────────
bool scanForPacket(uint8_t *bits, uint16_t bitCount,
                   uint8_t *out, uint8_t *outLen) {
  for (uint16_t offset = 0; offset + 64 <= bitCount; offset++) {
    for (uint8_t nbytes = 8; nbytes <= MAX_BYTES; nbytes++) {
      if (offset + (uint16_t)nbytes * 8 > bitCount) break;
      uint8_t candidate[MAX_BYTES] = {0};
      for (uint8_t b = 0; b < nbytes; b++) {
        uint8_t byte = 0;
        for (uint8_t bit = 0; bit < 8; bit++)
          byte = (byte << 1) | bits[offset + b*8 + bit];
        candidate[b] = byte;
      }
      // CRC valid + sync byte 0xAA
      if (crc8(candidate, nbytes-1) == candidate[nbytes-1] &&
          candidate[0] == 0xAA) {
        memcpy(out, candidate, nbytes);
        *outLen = nbytes;
        return true;
      }
    }
  }
  return false;
}

// ─── Decode & tampilkan data cuaca ───────────────────────────
void printWeather(uint8_t *d, uint8_t len) {
  if (len < 10) {
    Serial.println(F("[WARN] Paket terlalu pendek"));
    return;
  }

  // 16 arah angin presisi penuh (0=N, searah jarum jam, 22.5°/step)
  const char *dirs[] = {
    "N","NNE","NE","ENE","E","ESE","SE","SSE",
    "S","SSW","SW","WSW","W","WNW","NW","NNW"
  };

  // ── Decode semua field ──────────────────────────────────────
  uint8_t  sensorID  = d[1];
  uint16_t rawT      = ((uint16_t)(d[1] & 0x0F) << 8) | d[2];
  float    tempOut   = (rawT - 400) / 10.0f;
  uint8_t  humOut    = d[3];
  float    windSpeed = d[4] * 1.21f;
  float    windGust  = d[5] * 1.21f;
  float    rain      = d[6] * 0.3f;
  bool     battOK    = (d[7] & 0x80) != 0;
  uint8_t  channel   = (d[7] >> 4) & 0x07;
  uint8_t  windDirI  = d[8] & 0x0F;
  float    windDeg   = windDirI * 22.5f;

  // ── Print ───────────────────────────────────────────────────
  Serial.println(F("\n╔══════════════════════════════════════╗"));
  Serial.println(F(  "║     WEATHER STATION WH5300           ║"));
  Serial.println(F(  "╚══════════════════════════════════════╝"));
  Serial.printf(     "  Sensor ID  : 0x%02X  Ch: %d  Batt: %s\n",
                     sensorID, channel, battOK ? "OK" : "LOW ⚠");
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Suhu OUT   : %.1f °C\n",            tempOut);
  Serial.printf(     "  Humidity   : %d %%\n",               humOut);
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Angin      : %.1f km/h\n",           windSpeed);
  Serial.printf(     "  Gust       : %.1f km/h\n",           windGust);
  Serial.printf(     "  Arah       : %s (%.1f°)\n",          dirs[windDirI], windDeg);
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Rain       : %.1f mm\n",             rain);
  Serial.println(F(  "══════════════════════════════════════"));

  // ── Output JSON untuk IoT ───────────────────────────────────
  // Mudah di-parse oleh MQTT / Node-RED / Home Assistant
  Serial.println(F("[JSON]"));
  Serial.print(F("{"));
  Serial.printf("\"id\":\"0x%02X\",", sensorID);
  Serial.printf("\"ch\":%d,",         channel);
  Serial.printf("\"batt\":\"%s\",",   battOK ? "OK" : "LOW");
  Serial.printf("\"temp_out\":%.1f,", tempOut);
  Serial.printf("\"hum_out\":%d,",    humOut);
  Serial.printf("\"wind_speed\":%.1f,", windSpeed);
  Serial.printf("\"wind_gust\":%.1f,",  windGust);
  Serial.printf("\"wind_dir\":\"%s\",", dirs[windDirI]);
  Serial.printf("\"wind_deg\":%.1f,",   windDeg);
  Serial.printf("\"rain\":%.1f",        rain);
  Serial.println(F("}"));
}

// ─── Inisialisasi CC1101 ──────────────────────────────────────
void initCC1101() {
  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, 5);
  ELECHOUSE_cc1101.setGDO(GDO0_PIN, GDO2_PIN);
  ELECHOUSE_cc1101.Init();

  if (!ELECHOUSE_cc1101.getCC1101()) {
    Serial.println(F("[ERROR] CC1101 tidak terdeteksi! Cek wiring."));
    while (true) delay(1000);
  }

  ELECHOUSE_cc1101.setMHZ(433.92);
  ELECHOUSE_cc1101.setModulation(2);   // ASK/OOK
  ELECHOUSE_cc1101.setDRate(4.8);
  ELECHOUSE_cc1101.setRxBW(203.12);
  ELECHOUSE_cc1101.SetRx();
}

// ─── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println(F("\n══════════════════════════════════════"));
  Serial.println(F(  "  ESP32 CC1101 WH5300 Receiver v3"));
  Serial.println(F(  "══════════════════════════════════════"));

  initCC1101();
  Serial.println(F("[OK] CC1101 RX OOK 433.92 MHz"));

  pinMode(GDO2_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(GDO2_PIN), gdo2ISR, CHANGE);
  Serial.println(F("[OK] Interrupt aktif di GPIO 4"));
  Serial.println(F("[OK] Menunggu paket WH5300... (~48 detik antar TX)\n"));
}

// ─── Loop ─────────────────────────────────────────────────────
void loop() {
  uint32_t now = millis();

  // Commit via timeout — paket selesai jika tidak ada pulsa baru selama 30ms
  if (!gotPacket && pulseIdx >= 64) {
    noInterrupts();
    uint32_t lastMs = lastPulseMs;
    interrupts();
    if (now - lastMs >= PACKET_TIMEOUT) gotPacket = true;
  }

  // Bersihkan buffer noise kecil (< 64 pulsa, sudah lama tidak ada pulsa baru)
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

  // Watchdog: reinit CC1101 ke RX setiap menit
  static uint32_t lastReinit = 0;
  if (now - lastReinit > 60000) {
    lastReinit = now;
    ELECHOUSE_cc1101.SetRx();
  }

  if (!gotPacket) return;

  // ── Ambil buffer secara atomic ─────────────────────────────
  noInterrupts();
  uint16_t count = pulseIdx;
  uint32_t local[MAX_PULSES];
  memcpy(local, (const void*)pulseBuf, count * sizeof(uint32_t));
  pulseIdx  = 0;
  gotPacket = false;
  interrupts();

  // ── Konversi pulsa → bitstream ─────────────────────────────
  uint8_t bits[MAX_PULSES] = {0};
  uint16_t bitCount = pulsesToBits(local, count, bits, MAX_PULSES);
  if (bitCount < 64) return;

  // ── Scan bitstream → cari paket CRC valid ─────────────────
  uint8_t packet[MAX_BYTES] = {0};
  uint8_t pktLen = 0;

  if (!scanForPacket(bits, bitCount, packet, &pktLen)) return;

  // ── Tampilkan RAW bytes ────────────────────────────────────
  Serial.print(F("[RAW] "));
  for (int i = 0; i < pktLen; i++) Serial.printf("%02X ", packet[i]);
  Serial.println();

  // ── Decode & print ─────────────────────────────────────────
  printWeather(packet, pktLen);
}
