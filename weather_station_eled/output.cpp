#include "config.h"

static const char *WIND_DIRS[] = {
  "N","NNE","NE","ENE","E","ESE","SE","SSE",
  "S","SSW","SW","WSW","W","WNW","NW","NNW"
};

void printWeather(uint8_t *d, uint8_t len) {
  if (len < 10) {
    Serial.println(F("[WARN] Paket terlalu pendek"));
    return;
  }

  // Decode field WH5300
  uint8_t  sensorID  = d[1];
  uint16_t rawT      = ((uint16_t)(d[1] & 0x0F) << 8) | d[2];
  float    tempOut   = (rawT - TEMP_OFFSET) / TEMP_DIVISOR;
  uint8_t  humOut    = d[3];
  float    windSpeed = d[4] * WIND_FACTOR;
  float    windGust  = d[5] * WIND_FACTOR;
  uint8_t  rainRaw   = d[6];
  bool     battOK    = (d[7] & 0x80) != 0;
  uint8_t  channel   = (d[7] >> 4) & 0x07;
  uint8_t  windDirI  = d[8] & 0x0F;
  float    windDeg   = windDirI * WIND_DEG_STEP;

  float  rainDelta = calcRainDelta(rainRaw);
  String dtStr     = getDateTimeStr();
  String isoStr    = getISOStr();

  // Display
  Serial.println(F("\n╔══════════════════════════════════════╗"));
  Serial.println(F(  "║        WEATHER STATION ELED          ║"));
  Serial.println(F(  "╚══════════════════════════════════════╝"));
  Serial.printf(     "  Waktu      : %s\n",         dtStr.c_str());
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Sensor ID  : 0x%02X  Ch: %d  Batt: %s\n",
                     sensorID, channel, battOK ? "OK" : "LOW ⚠");
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Suhu OUT   : %.1f °C\n",   tempOut);
  Serial.printf(     "  Humidity   : %d %%\n",      humOut);
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Angin      : %.1f km/h\n",  windSpeed);
  Serial.printf(     "  Gust       : %.1f km/h\n",  windGust);
  Serial.printf(     "  Arah       : %s (%.1f°)\n", WIND_DIRS[windDirI], windDeg);
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Rain delta : %.1f mm\n",    rainDelta);
  Serial.printf(     "  Rain total : %.1f mm\n",    rainAccumulated);
  Serial.printf(     "  Rain raw   : %d tips\n",    rainRaw);
  Serial.println(F(  "  ─────────────────────────────────────"));
  Serial.printf(     "  Light      : %.1f lux\n",   luxValue);
  Serial.println(F(  "══════════════════════════════════════"));

  // JSON
  Serial.println(F("[JSON]"));
  Serial.print(F("{"));
  Serial.printf("\"datetime\":\"%s\",",  isoStr.c_str());
  Serial.printf("\"id\":\"0x%02X\",",    sensorID);
  Serial.printf("\"ch\":%d,",            channel);
  Serial.printf("\"batt\":\"%s\",",      battOK ? "OK" : "LOW");
  Serial.printf("\"temp_out\":%.1f,",    tempOut);
  Serial.printf("\"hum_out\":%d,",       humOut);
  Serial.printf("\"wind_speed\":%.1f,",  windSpeed);
  Serial.printf("\"wind_gust\":%.1f,",   windGust);
  Serial.printf("\"wind_dir\":\"%s\",",  WIND_DIRS[windDirI]);
  Serial.printf("\"wind_deg\":%.1f,",    windDeg);
  Serial.printf("\"rain_delta\":%.1f,",  rainDelta);
  Serial.printf("\"rain_total\":%.1f,",  rainAccumulated);
  Serial.printf("\"rain_raw\":%d,",      rainRaw);
  Serial.printf("\"light_lux\":%.1f",    luxValue);
  Serial.println(F("}"));
}
