class WeatherData {
  final String localDatetime;
  final int temperature;
  final int humidity;
  final String weatherDesc;
  final double windSpeed;
  final String windDirection;
  final int cloudCover;
  final String visibility;

  WeatherData({
    required this.localDatetime,
    required this.temperature,
    required this.humidity,
    required this.weatherDesc,
    required this.windSpeed,
    required this.windDirection,
    required this.cloudCover,
    required this.visibility,
  });

  factory WeatherData.fromJson(Map<String, dynamic> json) {
    return WeatherData(
      localDatetime: json['local_datetime']?.toString() ?? '-',

      temperature:
          int.tryParse(json['t']?.toString() ?? '') ?? 0,

      humidity:
          int.tryParse(json['hu']?.toString() ?? '') ?? 0,

      weatherDesc:
          json['weather_desc']?.toString() ?? '-',

      windSpeed:
          double.tryParse(json['ws']?.toString() ?? '') ?? 0.0,

      windDirection:
          json['wd']?.toString() ?? '-',

      cloudCover:
          int.tryParse(json['tcc']?.toString() ?? '') ?? 0,

      visibility:
          json['vs_text']?.toString() ?? '-',
    );
  }
}