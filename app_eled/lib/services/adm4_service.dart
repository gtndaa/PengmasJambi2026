import 'dart:convert';

import 'package:http/http.dart' as http;

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
  // =========================================================
  // MENCARI ADM4 SECARA OTOMATIS DARI NAMA LOKASI
  // =========================================================

  Future<Adm4Data> getAdm4FromLocation({
    required String village,
    required String district,
    required String city,
    required String province,
  }) async {
    print('================================');
    print('MENCARI ADM4 OTOMATIS');
    print('DESA      : $village');
    print('KECAMATAN : $district');
    print('KOTA      : $city');
    print('PROVINSI  : $province');
    print('================================');

    // =======================================================
    // 1. CARI KODE PROVINSI
    // =======================================================

    final provinceCode =
        await findProvinceCode(
      province,
    );

    print(
      'KODE PROVINSI: $provinceCode',
    );

    // =======================================================
    // 2. CARI KODE KOTA/KABUPATEN
    // =======================================================

    final cityCode =
        await findCityCode(
      provinceCode: provinceCode,
      cityName: city,
    );

    print(
      'KODE KOTA/KABUPATEN: $cityCode',
    );

    // =======================================================
    // 3. CARI KODE KECAMATAN
    // =======================================================

    final districtCode =
        await findDistrictCode(
      cityCode: cityCode,
      districtName: district,
    );

    print(
      'KODE KECAMATAN: $districtCode',
    );

    // =======================================================
    // 4. CARI KODE DESA/KELURAHAN
    // =======================================================

    final villageData =
        await findVillageCode(
      districtCode: districtCode,
      villageName: village,
    );

    print('================================');
    print('ADM4 BERHASIL DITEMUKAN');
    print('KODE ADM4: ${villageData['code']}');
    print('NAMA DESA: ${villageData['name']}');
    print('================================');

    return Adm4Data(
      adm4:
          villageData['code'].toString(),

      village:
          villageData['name'].toString(),

      district:
          district,

      city:
          city,

      province:
          province,
    );
  }

  // =========================================================
  // PROVINSI
  // =========================================================

  Future<String> findProvinceCode(
    String provinceName,
  ) async {
    final url = Uri.parse(
      'https://wilayah.id/api/provinces.json',
    );

    final response =
        await http.get(url);

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mengambil data provinsi',
      );
    }

    final data =
        jsonDecode(response.body);

    final provinces =
        data['data'] as List;

    final target =
        normalize(provinceName);

    for (final province in provinces) {
      final name =
          normalize(
        province['name'].toString(),
      );

      if (name == target) {
        return province['code'].toString();
      }
    }

    // Alias nama provinsi
    if (target == 'dki jakarta' ||
        target == 'jakarta') {
      return '31';
    }

    throw Exception(
      'Provinsi tidak ditemukan: '
      '$provinceName',
    );
  }

  // =========================================================
  // KOTA / KABUPATEN
  // =========================================================

  Future<String> findCityCode({
    required String provinceCode,
    required String cityName,
  }) async {
    final url = Uri.parse(
      'https://wilayah.id/api/regencies/'
      '$provinceCode.json',
    );

    final response =
        await http.get(url);

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mengambil data kota/kabupaten',
      );
    }

    final data =
        jsonDecode(response.body);

    final cities =
        data['data'] as List;

    final target =
        normalize(cityName);

    for (final city in cities) {
      final name =
          normalize(
        city['name'].toString(),
      );

      if (name == target ||
          name.contains(target) ||
          target.contains(name)) {
        return city['code'].toString();
      }
    }

    throw Exception(
      'Kota/kabupaten tidak ditemukan: '
      '$cityName',
    );
  }

  // =========================================================
  // KECAMATAN
  // =========================================================

  Future<String> findDistrictCode({
    required String cityCode,
    required String districtName,
  }) async {
    final url = Uri.parse(
      'https://wilayah.id/api/districts/'
      '$cityCode.json',
    );

    final response =
        await http.get(url);

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mengambil data kecamatan',
      );
    }

    final data =
        jsonDecode(response.body);

    final districts =
        data['data'] as List;

    final target =
        normalize(districtName);

    for (final district in districts) {
      final name =
          normalize(
        district['name'].toString(),
      );

      if (name == target ||
          name.contains(target) ||
          target.contains(name)) {
        return district['code'].toString();
      }
    }

    throw Exception(
      'Kecamatan tidak ditemukan: '
      '$districtName',
    );
  }

  // =========================================================
  // DESA / KELURAHAN → ADM4
  // =========================================================

  Future<Map<String, dynamic>> findVillageCode({
    required String districtCode,
    required String villageName,
  }) async {
    final url = Uri.parse(
      'https://wilayah.id/api/villages/'
      '$districtCode.json',
    );

    final response =
        await http.get(url);

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mengambil data desa/kelurahan',
      );
    }

    final data =
        jsonDecode(response.body);

    final villages =
        data['data'] as List;

    final target =
        normalize(villageName);

    for (final village in villages) {
      final name =
          normalize(
        village['name'].toString(),
      );

      if (name == target ||
          name.contains(target) ||
          target.contains(name)) {
        return {
          'code': village['code'],
          'name': village['name'],
        };
      }
    }

    throw Exception(
      'Desa/kelurahan tidak ditemukan: '
      '$villageName',
    );
  }

  // =========================================================
  // NORMALISASI NAMA
  // =========================================================

  String normalize(
    String value,
  ) {
    return value
        .toLowerCase()
        .trim()
        .replaceAll(
          'kota administrasi ',
          '',
        )
        .replaceAll(
          'kabupaten ',
          '',
        )
        .replaceAll(
          'kota ',
          '',
        );
  }
}