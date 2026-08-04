class SensorDataModel {
  final int logId;

  final DateTime datetime;

  final String deviceId;

  final int channel;

  final String batteryStatus;

  final double temperature;

  final double humidity;

  final double windSpeed;

  final double windGust;

  final String windDirection;

  final double windDegree;

  final double rainDelta;

  final double rainTotal;

  final double rainRaw;

  final double lightLux;

  const SensorDataModel({
    required this.logId,
    required this.datetime,
    required this.deviceId,
    required this.channel,
    required this.batteryStatus,
    required this.temperature,
    required this.humidity,
    required this.windSpeed,
    required this.windGust,
    required this.windDirection,
    required this.windDegree,
    required this.rainDelta,
    required this.rainTotal,
    required this.rainRaw,
    required this.lightLux,
  });

  factory SensorDataModel.fromJson(
    Map<String, dynamic> json,
  ) {
    return SensorDataModel(
      logId:
          _toInt(
        json['logId'],
      ),

      datetime:
          DateTime.tryParse(
        json['datetime']
                ?.toString() ??
            '',
      ) ??
              DateTime.now(),

      deviceId:
          json['id']
                  ?.toString() ??
              '',

      channel:
          _toInt(
        json['ch'],
      ),

      batteryStatus:
          json['batt']
                  ?.toString() ??
              'Unknown',

      temperature:
          _toDouble(
        json['temp_out'],
      ),

      humidity:
          _toDouble(
        json['hum_out'],
      ),

      windSpeed:
          _toDouble(
        json['wind_speed'],
      ),

      windGust:
          _toDouble(
        json['wind_gust'],
      ),

      windDirection:
          json['wind_dir']
                  ?.toString() ??
              '-',

      windDegree:
          _toDouble(
        json['wind_deg'],
      ),

      rainDelta:
          _toDouble(
        json['rain_delta'],
      ),

      rainTotal:
          _toDouble(
        json['rain_total'],
      ),

      rainRaw:
          _toDouble(
        json['rain_raw'],
      ),

      lightLux:
          _toDouble(
        json['light_lux'],
      ),
    );
  }

  static int _toInt(
    dynamic value,
  ) {
    if (value is int) {
      return value;
    }

    return int.tryParse(
          value?.toString() ??
              '',
        ) ??
        0;
  }

  static double _toDouble(
    dynamic value,
  ) {
    if (value is num) {
      return value.toDouble();
    }

    return double.tryParse(
          value?.toString() ??
              '',
        ) ??
        0;
  }
}