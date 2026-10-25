
import 'dart:convert';

import 'package:flutter/services.dart';

class Adm4Data {
  final String adm4;
  final String village;
  final String district;
  final String city;
  final String province;

  Adm4Data({
    required this.adm4,
    required this.village,
    required this.district,
    required this.city,
    required this.province,
  });
}

class Adm4Service {
  static const String _assetPath =
      'assets/data/indonesia_wilayah.json';

  // Data dimuat satu kali dan digunakan kembali.
  static Future<List<dynamic>>? _provincesFuture;

  Future<List<dynamic>> _loadProvinces() {
    return _provincesFuture ??= _readProvinces();
  }

  Future<List<dynamic>> _readProvinces() async {
    try {
      final jsonString =
          await rootBundle.loadString(_assetPath);

      final decoded = jsonDecode(jsonString);

      if (decoded is! Map<String, dynamic> ||
          decoded['provinces'] is! List) {
        throw const FormatException(
          'Format JSON wilayah tidak sesuai.',
        );
      }

      return decoded['provinces'] as List<dynamic>;
    } catch (_) {
      // Jika pemuatan gagal, izinkan percobaan ulang.
      _provincesFuture = null;
      rethrow;
    }
  }

  Future<Adm4Data> getAdm4FromLocation({
    required String village,
    required String district,
    required String city,
    required String province,
  }) async {
    final provinces = await _loadProvinces();

    final provinceData = _findByName(
      provinces,
      province,
      prefixesToRemove: const [],
    );

    if (provinceData == null) {
      throw Exception(
        'Provinsi tidak ditemukan: $province',
      );
    }

    final regencies =
        provinceData['regencies'] as List<dynamic>? ?? [];

    final cityData = _findByName(
      regencies,
      city,
    );

    if (cityData == null) {
      throw Exception(
        'Kota/kabupaten tidak ditemukan: $city',
      );
    }

    final districts =
        cityData['districts'] as List<dynamic>? ?? [];

    final districtData = _findByName(
      districts,
      district,
    );

    if (districtData == null) {
      throw Exception(
        'Kecamatan tidak ditemukan: $district',
      );
    }

    final villages =
        districtData['villages'] as List<dynamic>? ?? [];

    final villageData = _findByName(
      villages,
      village,
    );

    if (villageData == null) {
      throw Exception(
        'Desa/kelurahan tidak ditemukan: $village',
      );
    }

    return Adm4Data(
      adm4: villageData['code'].toString(),
      village: villageData['name'].toString(),
      district: districtData['name'].toString(),
      city: cityData['name'].toString(),
      province: provinceData['name'].toString(),
    );
  }

  Map<String, dynamic>? _findByName(
    List<dynamic> items,
    String input, {
    List<String> prefixesToRemove = const [
      'kabupaten ',
      'kota administrasi ',
      'kota ',
    ],
  }) {
    final target = _normalize(
      input,
      prefixesToRemove,
    );

    // Prioritaskan kecocokan nama yang tepat.
    for (final item in items) {
      if (item is! Map<String, dynamic>) continue;

      final name = item['name']?.toString() ?? '';

      if (_normalize(name, prefixesToRemove) == target) {
        return item;
      }
    }

    // Fallback untuk variasi nama yang tidak terlalu berbeda.
    for (final item in items) {
      if (item is! Map<String, dynamic>) continue;

      final name = item['name']?.toString() ?? '';
      final normalizedName =
          _normalize(name, prefixesToRemove);

      if (target.isNotEmpty &&
          (normalizedName.contains(target) ||
              target.contains(normalizedName))) {
        return item;
      }
    }

    return null;
  }

  String _normalize(
    String value,
    List<String> prefixesToRemove,
  ) {
    var result = value
        .toLowerCase()
        .trim()
        .replaceAll(RegExp(r'\s+'), ' ');

    for (final prefix in prefixesToRemove) {
      if (result.startsWith(prefix)) {
        result = result.substring(prefix.length).trim();
        break;
      }
    }

    return result;
  }
}
