#include "config.h"


void setup() {
  Serial.begin(115200);
  delay(300);

  Wire.begin(I2C_SDA, I2C_SCL);

  initRTC();
  initLux();
  initCC1101();

  pinMode(GDO2_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(GDO2_PIN), gdo2ISR, CHANGE);

  Serial.println(F("[OK] Interrupt aktif di GPIO 4"));
  Serial.println(F("[OK] Rain counter: delta mode (wrap-around handled)"));
  Serial.println(F("[OK] Menunggu paket WH5300... (~48 detik antar TX)\n"));
}

void loop() {
  uint32_t now = millis();
  updateLux();

  // Check for packet timeout
  if (!gotPacket && pulseIdx >= 64) {
    noInterrupts();
    uint32_t lastMs = lastPulseMs;
    interrupts();
    if (now - lastMs >= PACKET_TIMEOUT) gotPacket = true;
  }

  // Clean noise for short pulse sequences
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

  // Watchdog for CC1101 RX
  static uint32_t lastReinit = 0;
  if (now - lastReinit > 60000) {
    lastReinit = now;
    ELECHOUSE_cc1101.SetRx();
  }
  if (!gotPacket) return;

  // get a local copy of the pulse buffer and reset the global state
  noInterrupts();
  uint16_t count = pulseIdx;
  uint32_t local[MAX_PULSES];
  memcpy(local, (const void*)pulseBuf, count * sizeof(uint32_t));
  pulseIdx  = 0;
  gotPacket = false;
  interrupts();

  // Process the pulse sequence
  uint8_t  bits[MAX_PULSES] = {0};
  uint16_t bitCount = pulsesToBits(local, count, bits, MAX_PULSES);
  if (bitCount < 64) return;

  // Scan for a valid packet
  uint8_t packet[MAX_BYTES] = {0};
  uint8_t pktLen = 0;
  if (!scanForPacket(bits, bitCount, packet, &pktLen)) return;

  // Display the packet
  Serial.print(F("[RAW] "));
  for (int i = 0; i < pktLen; i++) Serial.printf("%02X ", packet[i]);
  Serial.println();
  printWeather(packet, pktLen);
}
