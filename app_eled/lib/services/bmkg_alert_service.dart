import 'dart:convert';

import 'package:http/http.dart' as http;
import 'package:xml/xml.dart';

import '../models/weather_alert_model.dart';

class BmkgAlertService {
  static const String rssUrl =
      'https://www.bmkg.go.id/alerts/nowcast/id/rss.xml';

  Future<List<WeatherAlert>> getActiveAlertsForLocation({
    required String province,
    required String district,
  }) async {
    final response = await http
        .get(
          Uri.parse(rssUrl),
        )
        .timeout(
          const Duration(seconds: 15),
        );

    if (response.statusCode != 200) {
      throw Exception(
        'Gagal mengambil peringatan BMKG. '
        'Status: ${response.statusCode}',
      );
    }

    final xmlString =
        utf8.decode(response.bodyBytes);

    final document =
        XmlDocument.parse(xmlString);

    final items =
        document.findAllElements('item');

    final List<WeatherAlert> allAlerts = [];

    for (final item in items) {
      final title =
          _getText(item, 'title');

      final description =
          _getText(item, 'description');

      final link =
          _getText(item, 'link');

      final author =
          _getText(item, 'author');

      final pubDate =
          _getText(item, 'pubDate');

      final guid =
          _getText(item, 'guid');

      allAlerts.add(
        WeatherAlert(
          id: guid.isNotEmpty
              ? guid
              : link,
          title: title,
          description: description,
          link: link,
          author: author,
          publishedAt: pubDate,
        ),
      );
    }

    final matchingAlerts =
        allAlerts.where((alert) {
      return _matchesLocation(
        alert: alert,
        province: province,
        district: district,
      );
    }).toList();

    return matchingAlerts;
  }

  String _getText(
    XmlElement element,
    String tag,
  ) {
    final result =
        element.findElements(tag);

    if (result.isEmpty) {
      return '';
    }

    return result.first.innerText.trim();
  }

  bool _matchesLocation({
    required WeatherAlert alert,
    required String province,
    required String district,
  }) {
    final normalizedTitle =
        _normalizeRegion(alert.title);

    final normalizedDescription =
        _normalizeRegion(
          alert.description,
        );

    final normalizedProvince =
        _normalizeRegion(province);

    final normalizedDistrict =
        _normalizeRegion(district);

    final provinceMatched =
        _containsPhrase(
      normalizedTitle,
      normalizedProvince,
    );

    final districtMatched =
        _containsPhrase(
      normalizedDescription,
      normalizedDistrict,
    );

    return provinceMatched &&
        districtMatched;
  }

  bool _containsPhrase(
    String text,
    String phrase,
  ) {
    return ' $text '.contains(
      ' $phrase ',
    );
  }

  String _normalizeRegion(
    String value,
  ) {
    return value
        .toLowerCase()
        .replaceAll(
          'kepulauan',
          'kep',
        )
        .replaceAll(
          'daerah istimewa',
          'di',
        )
        .replaceAll(
          'daerah khusus ibukota',
          'dki',
        )
        .replaceAll(
          RegExp(r'[^a-z0-9]+'),
          ' ',
        )
        .replaceAll(
          RegExp(r'\s+'),
          ' ',
        )
        .trim();
  }
}