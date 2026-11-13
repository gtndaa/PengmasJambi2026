import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;

import '../models/sensor_model.dart';
import 'api_service.dart';

/// Satuan waktu per titik pada grafik (dipilih otomatis)
enum HistoryGranularity {
  hour('hour', 'Per Jam', 'jam'),
  day('day', 'Per Hari', 'hari'),
  week('week', 'Per Minggu', 'minggu'),
  month('month', 'Per Bulan', 'bulan'),
  quarter('quarter', 'Per 3 Bulan', 'periode');

  final String apiValue;
  final String label; // teks untuk subjudul grafik
  final String noun; // kata untuk 1 titik data

  const HistoryGranularity(this.apiValue, this.label, this.noun);
}

class HistoryResult {
  /// 1 elemen per bucket. rainDelta = mm hujan di bucket itu (dari selisih
  /// rain_total, bukan jumlah rain_delta mentah),
  /// kolom lain = rata-rata di bucket itu.
  final List<SensorDataModel> rows;

  /// awal bucket (millisecondsSinceEpoch) -> jumlah jam yang punya data
  final Map<int, int> hoursByBucket;

  /// total hari yang punya data, dan di antaranya hari dengan hujan >= 1 mm
  final int daysWithData;
  final int rainyDays;

  const HistoryResult({
    required this.rows,
    required this.hoursByBucket,
    required this.daysWithData,
    required this.rainyDays,
  });
}

// ---------------------------------------------------------------------------
// Agregasi per jam (dihitung di isolate terpisah supaya UI tidak patah-patah)
// ---------------------------------------------------------------------------

/// Kolom yang dirata-rata. Urutannya dipakai sebagai indeks.
const _avgFields = [
  'temp_out',
  'hum_out',
  'wind_speed',
  'wind_gust',
  'wind_deg',
  'light_lux',
];

/// Satu jam data: [hujan, jumlah tiap kolom (6), banyaknya nilai tiap kolom (6)]
const _aggLen = 14; // 1 + 6 * 2 + 1

/// Indeks wind gust TERTINGGI di agregat. Gust ditampilkan sebagai nilai
/// maksimum per jam/hari/minggu/bulan (hembusan terkuat), bukan rata-rata.
const _gustMax = 13;

/// tanggal 'YYYY-MM-DD' -> jam (0-23) -> agregat jam itu
typedef _DayHours = Map<String, Map<int, List<double>>>;

double? _num(dynamic v) {
  if (v is num) return v.toDouble();
  if (v == null) return null;
  return double.tryParse(v.toString());
}

String _dayKey(DateTime d) =>
    '${d.year}-${d.month.toString().padLeft(2, '0')}-${d.day.toString().padLeft(2, '0')}';

/// Ubah body JSON /sensordata/history jadi agregat per jam.
/// Mengembalikan (jumlah baris mentah, agregat).
///
/// Curah hujan TIDAK diambil dari rain_delta: nilai itu rolling 60 menit
/// terakhir yang dikirim tiap menit, jadi satu tip akan terhitung ±60 kali.
/// Yang dipakai adalah rain_total (kumulatif, tersimpan di NVS alat):
/// hujan tiap record = rain_total − rain_total record sebelumnya, lalu
/// dijumlahkan ke jam record itu. Kalau selisihnya negatif (alat reset dan
/// total mulai dari 0 lagi), yang dipakai rain_total record itu saja.
/// Menjumlahkan selisih per record (bukan total_akhir − total_awal) membuat
/// reset di tengah jam/hari tidak merusak total.
(int, _DayHours) _aggregateBody(String body) {
  final decoded = jsonDecode(body);
  final raw = decoded is Map ? decoded['data'] : null;
  if (raw is! List) throw const FormatException('Format riwayat tidak valid');

  // pastikan urut waktu (backend sudah ORDER BY datetime ASC, ini hanya jaga-jaga)
  final records = <(DateTime, Map)>[];
  for (final item in raw) {
    if (item is! Map) continue;
    if (!ApiService.isFieldSensorRecord(item)) continue; // sensor uji coba
    if (!ApiService.isPlausibleRecord(item)) continue; // paket rusak
    var dt = DateTime.tryParse(item['datetime']?.toString() ?? '');
    if (dt == null) continue;
    if (dt.isUtc) dt = dt.toLocal();
    records.add((dt, item));
  }
  records.sort((x, y) => x.$1.compareTo(y.$1));

  final out = <String, Map<int, List<double>>>{};
  // rain_total record sebelumnya, PER SENSOR (id + channel). Tiap sensor punya
  // penghitung sendiri; kalau record dua sensor selang-seling lalu dihitung
  // bersama, selisihnya terbaca sebagai "reset" palsu dan hujan terhitung ganda.
  final prevTotal = <String, double>{};
  for (final (dt, item) in records) {
    final agg = out
        .putIfAbsent(_dayKey(dt), () => {})
        .putIfAbsent(dt.hour, () => List<double>.filled(_aggLen, 0));

    final total = _num(item['rain_total']);
    if (total != null) {
      final sensor = '${item['id']}|${item['ch']}';
      final prev = prevTotal[sensor];
      if (prev != null) {
        final diff = total - prev;
        agg[0] += diff < 0 ? total : diff;
      }
      prevTotal[sensor] = total;
    }
    for (var i = 0; i < _avgFields.length; i++) {
      final v = _num(item[_avgFields[i]]);
      if (v == null) continue;
      agg[1 + i] += v;
      agg[1 + _avgFields.length + i] += 1;
    }
    final gust = _num(item['wind_gust']);
    if (gust != null && gust > agg[_gustMax]) agg[_gustMax] = gust;
  }
  return (raw.length, out);
}

class SensorHistoryService {
  /// Batas baris dari backend per request (lihat getSensorRange di backend).
  static const _backendLimit = 20000;

  /// Rentang per request. Data ~1000 baris/hari, jadi 7 hari aman di bawah batas.
  static const _chunkDays = 7;

  /// Banyaknya request yang berjalan bersamaan.
  static const _parallel = 4;

  /// Cache hari yang sudah lewat (datanya tidak akan berubah lagi).
  /// Hari tanpa data disimpan sebagai map kosong.
  static final Map<String, Map<int, List<double>>> _cache = {};

  Future<HistoryResult> getHistory({
    required DateTime from,
    required DateTime to,
    required HistoryGranularity granularity,
  }) async {
    final days = <DateTime>[
      for (var d = DateTime(from.year, from.month, from.day);
          !d.isAfter(to);
          d = DateTime(d.year, d.month, d.day + 1))
        d,
    ];

    final today = _dayKey(DateTime.now());
    final hours = <String, Map<int, List<double>>>{};
    for (final d in days) {
      final k = _dayKey(d);
      final cached = _cache[k];
      if (cached != null && k != today) hours[k] = cached;
    }

    // Hari yang belum ada di cache, dipotong jadi beberapa rentang berurutan
    final chunks = <(DateTime, DateTime)>[];
    DateTime? start;
    DateTime? prev;
    for (final d in days) {
      final missing = !hours.containsKey(_dayKey(d));
      if (missing && start == null) start = d;
      final full = start != null && d.difference(start).inDays + 1 >= _chunkDays;
      if (start != null && (!missing || full)) {
        final end = missing ? d : prev!;
        chunks.add((start, end));
        start = null;
      }
      prev = d;
    }
    if (start != null) chunks.add((start, prev!));

    // Ambil semua rentang, maksimal [_parallel] request sekaligus
    var next = 0;
    Future<void> worker() async {
      while (next < chunks.length) {
        final (a, b) = chunks[next++];
        final fetched = await _fetchRange(a, b);
        for (var d = a; !d.isAfter(b); d = DateTime(d.year, d.month, d.day + 1)) {
          final k = _dayKey(d);
          final h = fetched[k] ?? <int, List<double>>{};
          hours[k] = h;
          if (k.compareTo(today) < 0) _cache[k] = h;
        }
      }
    }

    await Future.wait([
      for (var i = 0; i < _parallel && i < chunks.length; i++) worker(),
    ]);

    return _build(days, hours, granularity);
  }

  /// Ambil data mentah rentang [a, b]. Kalau kena batas baris backend,
  /// rentang dibelah dua lalu diambil ulang supaya tidak ada data yang hilang.
  Future<_DayHours> _fetchRange(DateTime a, DateTime b) async {
    // Backend memperlakukan `to` sebagai TIDAK termasuk (tanggalnya bergeser
    // lewat toISOString di zona WIB), jadi minta sampai b + 1 hari.
    // Kalau nanti backend sudah inklusif, hari ekstra ini cukup diabaikan
    // karena yang disimpan hanya hari a..b.
    final toExclusive = DateTime(b.year, b.month, b.day + 1);
    // Ambil juga 1 hari sebelum a sebagai acuan rain_total, supaya hujan di
    // record-record pertama hari a tetap terhitung. Hari acuan ini tidak
    // disimpan (yang disimpan hanya a..b).
    final baseline = DateTime(a.year, a.month, a.day - 1);
    final uri = Uri.parse('${ApiService.baseUrl}/sensordata/history').replace(
      queryParameters: {'from': _dayKey(baseline), 'to': _dayKey(toExclusive)},
    );

    final response = await http
        .get(uri, headers: {'Content-Type': 'application/json'})
        .timeout(const Duration(seconds: 60));

    if (response.statusCode < 200 || response.statusCode >= 300) {
      throw Exception(_errorMessage(response));
    }

    final (int count, _DayHours agg) result;
    try {
      result = await compute(_aggregateBody, response.body);
    } on FormatException catch (e) {
      throw Exception(e.message);
    }

    final span = b.difference(a).inDays + 1;
    if (result.$1 >= _backendLimit && span > 1) {
      final midEnd = DateTime(a.year, a.month, a.day + span ~/ 2 - 1);
      final midStart = DateTime(midEnd.year, midEnd.month, midEnd.day + 1);
      final parts = await Future.wait([
        _fetchRange(a, midEnd),
        _fetchRange(midStart, b),
      ]);
      return {...parts[0], ...parts[1]};
    }
    return result.$2;
  }

  DateTime _bucketStart(DateTime d, HistoryGranularity g) {
    switch (g) {
      case HistoryGranularity.hour:
        return DateTime(d.year, d.month, d.day, d.hour);
      case HistoryGranularity.day:
        return DateTime(d.year, d.month, d.day);
      case HistoryGranularity.week:
        return DateTime(d.year, d.month, d.day - (d.weekday - 1));
      case HistoryGranularity.month:
        return DateTime(d.year, d.month, 1);
      case HistoryGranularity.quarter:
        return DateTime(d.year, ((d.month - 1) ~/ 3) * 3 + 1, 1);
    }
  }

  HistoryResult _build(
    List<DateTime> days,
    Map<String, Map<int, List<double>>> hours,
    HistoryGranularity g,
  ) {
    // awal bucket -> agregat gabungan
    final buckets = <DateTime, List<double>>{};
    final hoursByBucket = <int, int>{};
    var daysWithData = 0;
    var rainyDays = 0;

    void add(DateTime key, List<double> agg, int hourCount) {
      final b = buckets.putIfAbsent(key, () => List<double>.filled(_aggLen, 0));
      for (var i = 0; i < _aggLen; i++) {
        if (i == _gustMax) {
          if (agg[i] > b[i]) b[i] = agg[i]; // maksimum, bukan jumlah
        } else {
          b[i] += agg[i];
        }
      }
      final ms = key.millisecondsSinceEpoch;
      hoursByBucket[ms] = (hoursByBucket[ms] ?? 0) + hourCount;
    }

    for (final d in days) {
      final dayHours = hours[_dayKey(d)];
      if (dayHours == null || dayHours.isEmpty) continue;

      if (g == HistoryGranularity.hour) {
        for (final e in dayHours.entries) {
          add(DateTime(d.year, d.month, d.day, e.key), e.value, 1);
        }
        continue;
      }

      // Rata-rata menurut waktu: tiap jam berbobot sama (rata-rata dari
      // rata-rata per jam). Kalau dijumlah per record, jam yang kebetulan
      // mengirim lebih banyak record (interval kirim tidak selalu sama)
      // akan terlalu berpengaruh. Hujan tetap dijumlahkan.
      final n = _avgFields.length;
      final dayAgg = List<double>.filled(_aggLen, 0);
      for (final h in dayHours.values) {
        dayAgg[0] += h[0];
        if (h[_gustMax] > dayAgg[_gustMax]) dayAgg[_gustMax] = h[_gustMax];
        for (var i = 0; i < n; i++) {
          if (h[1 + n + i] <= 0) continue;
          dayAgg[1 + i] += h[1 + i] / h[1 + n + i];
          dayAgg[1 + n + i] += 1;
        }
      }
      daysWithData++;
      if (dayAgg[0] >= 1.0) rainyDays++;
      add(_bucketStart(d, g), dayAgg, dayHours.length);
    }

    final keys = buckets.keys.toList()..sort();
    final n = _avgFields.length;
    final rows = <SensorDataModel>[
      for (final k in keys)
        () {
          final a = buckets[k]!;
          double avg(int i) => a[1 + n + i] > 0 ? a[1 + i] / a[1 + n + i] : 0;
          return SensorDataModel(
            logId: 0,
            datetime: k,
            deviceId: '',
            channel: 0,
            batteryStatus: '',
            temperature: avg(0),
            humidity: avg(1),
            windSpeed: avg(2),
            windGust: a[_gustMax], // tertinggi di bucket ini
            windDirection: '-',
            windDegree: avg(4),
            rainDelta: a[0],
            rainTotal: a[0],
            rainRaw: 0,
            lightLux: avg(5),
          );
        }(),
    ];

    return HistoryResult(
      rows: rows,
      hoursByBucket: hoursByBucket,
      daysWithData: daysWithData,
      rainyDays: rainyDays,
    );
  }

  String _errorMessage(http.Response r) {
    try {
      final body = jsonDecode(r.body);
      if (body is Map && body['message'] != null) {
        return body['message'].toString();
      }
    } catch (_) {}
    return 'Gagal mengambil riwayat. Status: ${r.statusCode}';
  }
}
