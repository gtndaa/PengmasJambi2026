
import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;

class BmkgService {
  static const String baseUrl =
      'https://api.bmkg.go.id/publik/prakiraan-cuaca';

  Future<Map<String, dynamic>> getWeatherData(
    String adm4,
  ) async {
    if (adm4.trim().isEmpty) {
      throw Exception('Kode ADM4 kosong.');
    }

    final url = Uri.parse(baseUrl).replace(
      queryParameters: {'adm4': adm4.trim()},
    );

    debugPrint('[BMKG] Mulai request');
    debugPrint('[BMKG] ADM4: $adm4');
    debugPrint('[BMKG] URL: $url');

    try {
      final response = await http
          .get(
            url,
            headers: {'Accept': 'application/json'},
          )
          .timeout(const Duration(seconds: 20));

      debugPrint('[BMKG] HTTP ${response.statusCode}');
      debugPrint(
        '[BMKG] Respons: ${response.body.substring(
          0,
          response.body.length > 1000
              ? 1000
              : response.body.length,
        )}',
      );

      if (response.statusCode != 200) {
        throw Exception(
          'BMKG mengembalikan HTTP ${response.statusCode}',
        );
      }

      final decoded = jsonDecode(response.body);

      if (decoded is! Map<String, dynamic>) {
        throw Exception(
          'Format respons BMKG bukan objek JSON.',
        );
      }

      debugPrint('[BMKG] JSON berhasil diproses');
      return decoded;
    } on TimeoutException {
      debugPrint('[BMKG] Request timeout setelah 20 detik');
      throw Exception(
        'Request cuaca timeout. Periksa koneksi internet.',
      );
    } on FormatException catch (e) {
      debugPrint('[BMKG] JSON tidak valid: $e');
      throw Exception('Format data cuaca tidak valid.');
    } catch (e, stackTrace) {
      debugPrint('[BMKG] Request gagal: $e');
      debugPrintStack(stackTrace: stackTrace);
      rethrow;
    }
  }
}
