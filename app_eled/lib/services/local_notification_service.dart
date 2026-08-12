import 'package:flutter/foundation.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';

import '../models/weather_alert_model.dart';

class LocalNotificationService {
  // =====================================================
  // PLUGIN INSTANCE
  // =====================================================

  static final FlutterLocalNotificationsPlugin _plugin =
      FlutterLocalNotificationsPlugin();

  static bool _initialized = false;

  // =====================================================
  // PLATFORM CHECK
  // =====================================================

  /// Background weather notification saat ini
  /// hanya dikonfigurasi untuk Android.
  static bool get _isAndroid {
    if (kIsWeb) {
      return false;
    }

    return defaultTargetPlatform == TargetPlatform.android;
  }

  // =====================================================
  // INITIALIZE
  // =====================================================

  static Future<void> initialize() async {
    if (!_isAndroid || _initialized) {
      return;
    }

    const androidSettings =
        AndroidInitializationSettings(
      '@mipmap/ic_launcher',
    );

    const initializationSettings =
        InitializationSettings(
      android: androidSettings,
    );

    await _plugin.initialize(
      settings: initializationSettings,
    );

    _initialized = true;
  }

  // =====================================================
  // REQUEST PERMISSION
  // =====================================================

  /// Meminta izin notification pada Android 13+.
  ///
  /// Pada versi Android yang lebih lama,
  /// method ini tidak akan menampilkan permission dialog.
  static Future<void> requestPermission() async {
    if (!_isAndroid) {
      return;
    }

    await initialize();

    final androidPlugin =
        _plugin.resolvePlatformSpecificImplementation<
            AndroidFlutterLocalNotificationsPlugin>();

    await androidPlugin
        ?.requestNotificationsPermission();
  }

  // =====================================================
  // SHOW WEATHER ALERT
  // =====================================================

  /// Menampilkan warning BMKG sebagai
  /// notification Android.
  static Future<void> showWeatherAlert(
    WeatherAlert alert,
  ) async {
    if (!_isAndroid) {
      return;
    }

    await initialize();

    final bigTextStyle =
        BigTextStyleInformation(
      alert.description,
      contentTitle: alert.title,
    );

    final androidDetails =
        AndroidNotificationDetails(
      'weather_alerts',
      'Peringatan Cuaca',
      channelDescription:
          'Peringatan dini cuaca BMKG',
      importance: Importance.max,
      priority: Priority.high,
      playSound: true,
      enableVibration: true,
      styleInformation: bigTextStyle,
    );

    final notificationDetails =
        NotificationDetails(
      android: androidDetails,
    );

    await _plugin.show(
      id: _createNotificationId(alert.id),
      title: '⚠️ ${alert.title}',
      body: alert.description,
      notificationDetails:
          notificationDetails,
      payload: alert.link,
    );
  }

  // =====================================================
  // NOTIFICATION ID
  // =====================================================

  /// Mengubah ID alert BMKG menjadi integer
  /// yang stabil untuk Android notification.
  ///
  /// Alert dengan ID yang sama akan menggunakan
  /// notification ID yang sama.
  static int _createNotificationId(
    String value,
  ) {
    int hash = 0;

    for (final codeUnit in value.codeUnits) {
      hash =
          ((hash * 31) + codeUnit) &
              0x7fffffff;
    }

    return hash;
  }
}