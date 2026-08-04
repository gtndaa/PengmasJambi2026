import 'dart:convert';
import 'package:http/http.dart' as http;

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
}

class WilayahService {
  static const String baseUrl =
      'https://wilayah.id/api';

  // =====================================================
  // GET PROVINSI
  // =====================================================

  Future<List<WilayahData>>
      getProvinces() async {
    final url = Uri.parse(
      '$baseUrl/provinces.json',
    );

    final response =
        await http.get(url);

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mengambil data provinsi',
      );
    }

    final json =
        jsonDecode(response.body);

    final List data =
        json['data'];

    return data
        .map(
          (item) =>
              WilayahData.fromJson(item),
        )
        .toList();
  }

  // =====================================================
  // GET KABUPATEN / KOTA
  // =====================================================

  Future<List<WilayahData>>
      getRegencies(
    String provinceCode,
  ) async {
    final url = Uri.parse(
      '$baseUrl/regencies/'
      '$provinceCode.json',
    );

    final response =
        await http.get(url);

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mengambil data kabupaten/kota',
      );
    }

    final json =
        jsonDecode(response.body);

    final List data =
        json['data'];

    return data
        .map(
          (item) =>
              WilayahData.fromJson(item),
        )
        .toList();
  }

  // =====================================================
  // GET KECAMATAN
  // =====================================================

  Future<List<WilayahData>>
      getDistricts(
    String regencyCode,
  ) async {
    final url = Uri.parse(
      '$baseUrl/districts/'
      '$regencyCode.json',
    );

    final response =
        await http.get(url);

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mengambil data kecamatan',
      );
    }

    final json =
        jsonDecode(response.body);

    final List data =
        json['data'];

    return data
        .map(
          (item) =>
              WilayahData.fromJson(item),
        )
        .toList();
  }

  // =====================================================
  // GET KELURAHAN / DESA
  // =====================================================

  Future<List<WilayahData>>
      getVillages(
    String districtCode,
  ) async {
    final url = Uri.parse(
      '$baseUrl/villages/'
      '$districtCode.json',
    );

    final response =
        await http.get(url);

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mengambil data kelurahan/desa',
      );
    }

    final json =
        jsonDecode(response.body);

    final List data =
        json['data'];

    return data
        .map(
          (item) =>
              WilayahData.fromJson(item),
        )
        .toList();
  }
}