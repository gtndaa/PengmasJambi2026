import 'package:flutter/material.dart';

import 'sensor_model.dart';

enum SensorMetric {
  temperature('Suhu', '°C', Icons.thermostat, false),
  humidity('Kelembapan', '%', Icons.water_drop_outlined, false),
  light('Intensitas Cahaya', 'lux', Icons.light_mode_outlined, false),
  rain('Curah Hujan 1 Jam Terakhir', 'mm', Icons.umbrella_outlined, true),
  wind('Kecepatan Angin', 'km/j', Icons.air, false),
  gust('Wind Gust', 'km/j', Icons.speed, false);

  final String label;
  final String unit;
  final IconData icon;

  /// true = dijumlahkan per hari (hujan), false = dirata-rata per hari
  final bool isSum;

  const SensorMetric(this.label, this.unit, this.icon, this.isSum);

  double valueOf(SensorDataModel d) {
    switch (this) {
      case SensorMetric.temperature:
        return d.temperature;
      case SensorMetric.humidity:
        return d.humidity;
      case SensorMetric.light:
        return d.lightLux;
      case SensorMetric.rain:
        return d.rainDelta;
      case SensorMetric.wind:
        return d.windSpeed * 3.6; // m/s -> km/j
      case SensorMetric.gust:
        return d.windGust * 3.6;
    }
  }
}