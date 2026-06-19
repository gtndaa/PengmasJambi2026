#include <SPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define CC1101_SCK   18
#define CC1101_MISO  19
#define CC1101_MOSI  23
#define CC1101_CS    5
#define CC1101_GDO0  4

byte rxBuffer[64];

void setup() {
  Serial.begin(115200);

  SPI.begin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CS);

  ELECHOUSE_cc1101.setSpiPin(
      CC1101_SCK,
      CC1101_MISO,
      CC1101_MOSI,
      CC1101_CS
  );

  if (!ELECHOUSE_cc1101.getCC1101()) {
    Serial.println("CC1101 tidak terdeteksi");
    while (1);
  }

  ELECHOUSE_cc1101.Init();

  // Setting umum weather station 433 MHz
  ELECHOUSE_cc1101.setMHZ(433.92);

  // Coba GFSK dulu
  ELECHOUSE_cc1101.setModulation(1);

  ELECHOUSE_cc1101.setRxBW(270.83);
  ELECHOUSE_cc1101.setDRate(9.6);

  ELECHOUSE_cc1101.SetRx();

  Serial.println("Listening...");
}

void loop() {

  if (ELECHOUSE_cc1101.CheckRxFifo()) {

    byte len = ELECHOUSE_cc1101.ReceiveData(rxBuffer);

    Serial.print("RSSI: ");
    Serial.print(ELECHOUSE_cc1101.getRssi());
    Serial.print(" dBm  ");

    Serial.print("LEN: ");
    Serial.print(len);
    Serial.print(" DATA: ");

    for (int i = 0; i < len; i++) {
      if (rxBuffer[i] < 0x10)
        Serial.print('0');

      Serial.print(rxBuffer[i], HEX);
      Serial.print(' ');
    }

    Serial.println();

    ELECHOUSE_cc1101.SetRx();
  }
}