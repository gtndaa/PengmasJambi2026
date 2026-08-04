import 'dart:convert';

import 'package:flutter/services.dart';

class WilayahData {
  final String code;
  final String name;

  WilayahData({
    required this.code,
    required this.name,
  });

  factory WilayahData.fromJson(
    Map<String, dynamic> json,
  ) {
    return WilayahData(
      code: json['code'].toString(),
      name: json['name'].toString(),
    );
  }

  @override
  String toString() {
    return '$name ($code)';
  }
}

class WilayahLocalService {
  Map<String, dynamic>? _database;

  // =====================================================
  // LOAD DATABASE JSON
  // =====================================================

  Future<void> loadDatabase() async {
    final jsonString =
        await rootBundle.loadString(
      'assets/data/indonesia_wilayah.json',
    );

    _database =
        jsonDecode(jsonString);

    print('================================');
    print('DATABASE WILAYAH BERHASIL DIMUAT');
    print('================================');
  }

  // =====================================================
  // CEK DATABASE
  // =====================================================

  bool get isLoaded {
    return _database != null;
  }

  // =====================================================
  // PROVINSI
  // =====================================================

  List<WilayahData> getProvinces() {
    if (_database == null) {
      return [];
    }

    final provinces =
        _database!['provinces'] as List;

    return provinces
        .map(
          (item) =>
              WilayahData.fromJson(
            item,
          ),
        )
        .toList();
  }

  // =====================================================
  // KABUPATEN / KOTA
  // =====================================================

  List<WilayahData> getRegencies(
    String provinceCode,
  ) {
    if (_database == null) {
      return [];
    }

    final provinces =
        _database!['provinces'] as List;

    for (final province in provinces) {
      if (province['code']
              .toString() ==
          provinceCode) {
        final regencies =
            province['regencies'] as List;

        return regencies
            .map(
              (item) =>
                  WilayahData.fromJson(
                item,
              ),
            )
            .toList();
      }
    }

    return [];
  }

  // =====================================================
  // KECAMATAN
  // =====================================================

  List<WilayahData> getDistricts(
    String regencyCode,
  ) {
    if (_database == null) {
      return [];
    }

    final provinces =
        _database!['provinces'] as List;

    for (final province in provinces) {
      final regencies =
          province['regencies'] as List;

      for (final regency in regencies) {
        if (regency['code']
                .toString() ==
            regencyCode) {
          final districts =
              regency['districts']
                  as List;

          return districts
              .map(
                (item) =>
                    WilayahData.fromJson(
                  item,
                ),
              )
              .toList();
        }
      }
    }

    return [];
  }

  // =====================================================
  // KELURAHAN / DESA
  // =====================================================

  List<WilayahData> getVillages(
    String districtCode,
  ) {
    if (_database == null) {
      return [];
    }

    final provinces =
        _database!['provinces'] as List;

    for (final province in provinces) {
      final regencies =
          province['regencies'] as List;

      for (final regency in regencies) {
        final districts =
            regency['districts']
                as List;

        for (final district in districts) {
          if (district['code']
                  .toString() ==
              districtCode) {
            final villages =
                district['villages']
                    as List;

            return villages
                .map(
                  (item) =>
                      WilayahData.fromJson(
                    item,
                  ),
                )
                .toList();
          }
        }
      }
    }

    return [];
  }
}