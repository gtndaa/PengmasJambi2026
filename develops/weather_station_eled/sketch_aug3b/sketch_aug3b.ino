#include <Wire.h>
#include <BH1750.h>

#define SDA_PIN 21
#define SCL_PIN 22

BH1750 lightMeter;

void setup() {
  Serial.begin(115200);

  // Inisialisasi I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Inisialisasi BH1750
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 berhasil diinisialisasi.");
  } else {
    Serial.println("Gagal mendeteksi BH1750!");
    while (1);
  }
}

void loop() {
  float lux = lightMeter.readLightLevel();

  if (lux >= 0) {
    Serial.print("Intensitas Cahaya: ");
    Serial.print(lux);
    Serial.println(" lux");
  } else {
    Serial.println("Gagal membaca sensor.");
  }

  delay(1000);
}