import 'dart:convert';

import 'package:http/http.dart' as http;

class DeviceConfigService {
  // =====================================================
  // KIRIM KONFIGURASI KE CLOUD
  // =====================================================

  Future<void> sendDeviceConfig({
    required String serverURL,
    required String apiKey,
    required Map<String, dynamic> config,
  }) async {
    final uri = Uri.parse(serverURL);

    final response = await http.post(
      uri,
      headers: {
        'Content-Type': 'application/json',
        'x-api-key': apiKey,
      },
      body: jsonEncode(config),
    );

    print('================================');
    print('RESPONSE DEVICE CONFIGURATION');
    print('STATUS CODE: ${response.statusCode}');
    print('BODY: ${response.body}');
    print('================================');

    if (response.statusCode < 200 ||
        response.statusCode >= 300) {
      throw Exception(
        'Server mengembalikan status '
        '${response.statusCode}',
      );
    }
  }
}