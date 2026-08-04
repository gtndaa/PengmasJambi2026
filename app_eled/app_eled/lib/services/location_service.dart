import 'dart:convert';

import 'package:http/http.dart' as http;
import 'package:geolocator/geolocator.dart';

class LocationData {
  final double latitude;
  final double longitude;

  final String village;
  final String district;
  final String city;
  final String province;

  LocationData({
    required this.latitude,
    required this.longitude,
    required this.village,
    required this.district,
    required this.city,
    required this.province,
  });
}

class LocationService {
  Future<LocationData> getCurrentLocation() async {
    bool serviceEnabled =
        await Geolocator.isLocationServiceEnabled();

    if (!serviceEnabled) {
      throw Exception(
        'Layanan lokasi tidak aktif',
      );
    }

    LocationPermission permission =
        await Geolocator.checkPermission();

    if (permission ==
        LocationPermission.denied) {
      permission =
          await Geolocator.requestPermission();
    }

    if (permission ==
            LocationPermission.denied ||
        permission ==
            LocationPermission.deniedForever) {
      throw Exception(
        'Izin lokasi ditolak',
      );
    }

    final position =
        await Geolocator.getCurrentPosition(
      desiredAccuracy:
          LocationAccuracy.high,
    );

    final latitude =
        position.latitude;

    final longitude =
        position.longitude;

    final address =
        await reverseGeocode(
      latitude,
      longitude,
    );

    return LocationData(
      latitude: latitude,
      longitude: longitude,

      village:
          address['village'] ?? '',

      district:
          address['district'] ??
          address['suburb'] ??
          address['city_district'] ??
          '',

      city:
          address['city'] ??
          address['municipality'] ??
          address['county'] ??
          '',

      province:
          address['state'] ??
          '',
    );
  }

  Future<Map<String, dynamic>> reverseGeocode(
    double latitude,
    double longitude,
  ) async {
    final url = Uri.parse(
      'https://nominatim.openstreetmap.org/reverse'
      '?lat=$latitude'
      '&lon=$longitude'
      '&format=json'
      '&zoom=18'
      '&addressdetails=1',
    );

    final response =
        await http.get(
      url,
      headers: {
        'User-Agent':
            'AppELED/1.0',
      },
    );

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mendapatkan lokasi',
      );
    }

    final data =
        jsonDecode(response.body);

    final address =
        data['address'];

    print('================================');
    print('INFORMASI LOKASI');
    print(
      'DESA/KELURAHAN: '
      '${address['village'] ?? ''}',
    );
    print(
      'KECAMATAN: '
      '${address['district'] ?? address['suburb'] ?? address['city_district'] ?? ''}',
    );
    print(
      'KOTA/KABUPATEN: '
      '${address['city'] ?? address['municipality'] ?? address['county'] ?? ''}',
    );
    print(
      'PROVINSI: '
      '${address['state'] ?? ''}',
    );
    print(
      'LATITUDE: $latitude',
    );
    print(
      'LONGITUDE: $longitude',
    );
    print('================================');

    return address;
  }
}