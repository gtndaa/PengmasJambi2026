#ifndef API_CLIENT_H
#define API_CLIENT_H

void sendSensorData(
    String datetime,
    String id,
    int ch,
    float batt,
    float temp_out,
    float hum_out,
    float wind_speed,
    float wind_gust,
    String wind_dir,
    int wind_deg,
    float rain_delta,
    float rain_total,
    int rain_raw,
    float light_lux
);

#endif