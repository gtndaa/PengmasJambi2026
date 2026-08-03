import 'dart:convert';
import 'package:http/http.dart' as http;

class BmkgService {
  static const String baseUrl =
      'https://api.bmkg.go.id/publik/prakiraan-cuaca';

  Future<Map<String, dynamic>> getWeatherData(
    String adm4,
  ) async {
    final url = Uri.parse(
      '$baseUrl?adm4=$adm4',
    );

    print('================================');
    print('REQUEST DATA CUACA BMKG');
    print('ADM4: $adm4');
    print('URL: $url');
    print('================================');

    final response = await http.get(url);

    print('STATUS CODE: ${response.statusCode}');

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mengambil data BMKG. '
        'Status: ${response.statusCode}',
      );
    }

    final data = jsonDecode(response.body);

    return data;
  }
}