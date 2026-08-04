class SensorModel {

  final double temperature;

  final double humidity;

  final double pressure;

  final double rainfall;

  SensorModel({

    required this.temperature,

    required this.humidity,

    required this.pressure,

    required this.rainfall,

  });

  factory SensorModel.fromJson(Map<String, dynamic> json){

    return SensorModel(

      temperature: json["temperature"] ?? 0,

      humidity: json["humidity"] ?? 0,

      pressure: json["pressure"] ?? 0,

      rainfall: json["rainfall"] ?? 0,

    );

  }

}