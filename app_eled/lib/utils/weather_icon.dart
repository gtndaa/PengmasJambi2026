import 'package:flutter/material.dart';

/// Malam = 18.00 sampai 05.59.
bool isNightHour(int hour) => hour >= 18 || hour < 6;

/// Cek malam dari `local_datetime` BMKG (mis. "2026-11-12 20:00:00").
/// Kalau tidak bisa dibaca, pakai jam sekarang.
bool isNightAt(String? localDatetime) {
  final dt = DateTime.tryParse(localDatetime ?? '') ?? DateTime.now();
  return isNightHour(dt.hour);
}

/// Ikon cuaca dari deskripsi BMKG. Saat malam, matahari/awan diganti bulan.
IconData weatherIcon(String weather, {required bool night}) {
  final text = weather.toLowerCase();

  if (text.contains('petir') || text.contains('badai')) {
    return Icons.thunderstorm;
  }
  if (text.contains('hujan')) {
    return Icons.umbrella;
  }
  if (text.contains('cerah')) {
    if (!night) return Icons.wb_sunny;
    // "Cerah Berawan" -> bulan dengan awan, "Cerah" -> bulan saja
    return text.contains('berawan') ? Icons.nights_stay : Icons.nightlight_round;
  }
  if (text.contains('berawan')) {
    return night ? Icons.nights_stay : Icons.cloud;
  }
  return night ? Icons.nights_stay : Icons.cloud_outlined;
}

/// Warna ikon yang serasi dengan [weatherIcon].
Color weatherIconColor(String weather, {required bool night}) {
  final text = weather.toLowerCase();

  if (text.contains('petir') || text.contains('badai')) {
    return Colors.deepPurple;
  }
  if (text.contains('hujan')) {
    return Colors.blue;
  }
  if (night) {
    return const Color(0xFF5C6BC0); // indigo untuk bulan
  }
  if (text.contains('cerah')) {
    return Colors.amber;
  }
  return const Color.fromARGB(255, 55, 115, 255);
}
