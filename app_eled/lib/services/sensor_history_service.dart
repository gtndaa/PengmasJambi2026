import 'dart:convert';
import 'dart:math';

import 'package:http/http.dart' as http;

import '../models/sensor_model.dart';
import 'api_service.dart';

class SensorHistoryService {
  // Ubah ke false setelah endpoint history di backend tersedia.
  static const bool useMock = true;

  Future<List<SensorDataModel>> getHistory(int days) async {
    final now = DateTime.now();
    final from = DateTime(now.year, now.month, now.day)
        .subtract(Duration(days: days - 1));

    if (useMock) {
      await Future.delayed(const Duration(milliseconds: 400));
      return _mock(from, now);
    }

    final uri = Uri.parse('${ApiService.baseUrl}/sensordata/history')
        .replace(queryParameters: {'from': _fmt(from), 'to': _fmt(now)});

    final response = await http.get(
      uri,
      headers: {'Content-Type': 'application/json'},
    );

    if (response.statusCode < 200 || response.statusCode >= 300) {
      throw Exception('Gagal mengambil riwayat. Status: ${response.statusCode}');
    }

    final decoded = jsonDecode(response.body);
    final raw = decoded is Map ? decoded['data'] : decoded;
    if (raw is! List) {
      throw Exception('Format riwayat tidak valid');
    }

    return raw
        .whereType<Map<String, dynamic>>()
        .map(SensorDataModel.fromJson)
        .toList();
  }

  List<SensorDataModel> _mock(DateTime from, DateTime now) {
    final rnd = Random(from.day * 31 + from.month);
    final list = <SensorDataModel>[];
    var t = from;
    var id = 1;

    while (!t.isAfter(now)) {
      final swing = sin((t.hour - 6) / 24 * 2 * pi);
      final rainy = rnd.nextDouble() < 0.12;

      list.add(SensorDataModel(
        logId: id++,
        datetime: t,
        deviceId: 'mock',
        channel: 1,
        batteryStatus: 'OK',
        temperature: 27 + 5 * swing + rnd.nextDouble(),
        humidity: (80 - 15 * swing + rnd.nextDouble() * 5)
            .clamp(40, 100)
            .toDouble(),
        windSpeed: 0.5 + rnd.nextDouble() * 2.5,
        windGust: 1.5 + rnd.nextDouble() * 4,
        windDirection: 'N',
        windDegree: 0,
        rainDelta: rainy ? rnd.nextDouble() * 9 : 0,
        rainTotal: 0,
        rainRaw: 0,
        lightLux: max(0.0, 60000 * swing) + rnd.nextDouble() * 500,
      ));

      t = t.add(const Duration(hours: 3));
    }
    return list;
  }

  String _fmt(DateTime d) =>
      '${d.year}-${d.month.toString().padLeft(2, '0')}-${d.day.toString().padLeft(2, '0')}';
}