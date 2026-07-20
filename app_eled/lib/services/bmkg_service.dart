import 'dart:convert';

import 'package:http/http.dart'
    as http;

class BmkgService {
  // =====================================================
  // API BMKG
  // =====================================================

  Future<Map<String, dynamic>>
      getWeatherData(
    String adm4,
  ) async {
    final url = Uri.parse(
      'https://api.bmkg.go.id/'
      'publik/prakiraan-cuaca'
      '?adm4=$adm4',
    );

    print('================================');
    print('REQUEST API BMKG');
    print('ADM4: $adm4');
    print('URL: $url');
    print('================================');

    final response =
        await http.get(url);

    print(
      'STATUS CODE BMKG: '
      '${response.statusCode}',
    );

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mengambil data BMKG. '
        'Status code: '
        '${response.statusCode}',
      );
    }

    final data =
        jsonDecode(response.body);

    return data;
  }
}