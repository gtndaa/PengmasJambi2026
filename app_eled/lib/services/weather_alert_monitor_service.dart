import 'package:shared_preferences/shared_preferences.dart';

import '../models/weather_alert_model.dart';
import 'bmkg_alert_service.dart';
import 'local_notification_service.dart';

class WeatherAlertMonitorService {
  static const String _notifiedIdsKey =
      'notified_weather_alert_ids';

  static Future<int> checkAndNotify() async {
    final prefs =
        await SharedPreferences.getInstance();

    // Penting untuk background isolate:
    // ambil data terbaru dari native storage.
    await prefs.reload();

    final province =
        prefs.getString('selected_province') ?? '';

    final district =
        prefs.getString('selected_district') ?? '';

    if (province.isEmpty) {
      return 0;
    }

    final service = BmkgAlertService();

    final List<WeatherAlert> allAlerts = [];

    // ==============================
    // 1. BMKG Nowcast
    // ==============================
    try {
      final nowcastAlerts =
          await service.getActiveAlertsForLocation(
        province: province,
        district: district,
      );

      allAlerts.addAll(nowcastAlerts);
    } catch (e) {
      print(
        'Gagal mengambil BMKG Nowcast: $e',
      );
    }

    if (allAlerts.isEmpty) {
      return 0;
    }

    // Refresh lagi sebelum baca history notif.
    await prefs.reload();

    final oldIds =
        prefs.getStringList(_notifiedIdsKey) ??
            <String>[];

    final seenIds = oldIds.toSet();

    final List<String> newIds = [];

    int notificationCount = 0;

    for (final alert in allAlerts) {
      // Jangan kirim kalau id kosong
      // atau alert sudah pernah dikirim.
      if (alert.id.isEmpty ||
          seenIds.contains(alert.id)) {
        continue;
      }

      await LocalNotificationService
          .showWeatherAlert(alert);

      seenIds.add(alert.id);
      newIds.add(alert.id);

      notificationCount++;
    }

    // Simpan history supaya notif yang sama
    // tidak muncul terus setiap 15 menit.
    if (newIds.isNotEmpty) {
      final updatedIds = [
        ...oldIds,
        ...newIds,
      ];

      // Batasi history maksimal 200 ID.
      final limitedIds =
          updatedIds.length > 200
              ? updatedIds.sublist(
                  updatedIds.length - 200,
                )
              : updatedIds;

      await prefs.setStringList(
        _notifiedIdsKey,
        limitedIds,
      );
    }

    return notificationCount;
  }

  static Future<void>
      resetNotificationHistory() async {
    final prefs =
        await SharedPreferences.getInstance();

    await prefs.remove(
      _notifiedIdsKey,
    );
  }
}