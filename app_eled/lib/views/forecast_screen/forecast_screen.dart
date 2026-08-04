import 'package:flutter/material.dart';

class ForecastScreen
    extends StatelessWidget {
  const ForecastScreen({
    super.key,
  });

  @override
  Widget build(
    BuildContext context,
  ) {
    final weatherData =
        ModalRoute.of(context)
            ?.settings
            .arguments
            as Map<String, dynamic>?;

    return Scaffold(
      appBar: AppBar(
        title:
            const Text(
          'Prakiraan Cuaca',
        ),
      ),

      body:
          weatherData == null
              ? const Center(
                  child:
                      Text(
                    'Data cuaca tidak tersedia.',
                  ),
                )
              : _buildForecast(
                  weatherData,
                ),
    );
  }

  Widget _buildForecast(
    Map<String, dynamic>
        weatherData,
  ) {
    final data =
        weatherData['data'];

    if (data is! List ||
        data.isEmpty) {
      return const Center(
        child:
            Text(
          'Data cuaca tidak tersedia.',
        ),
      );
    }

    final cuaca =
        data[0]['cuaca'];

    final List<dynamic>
        forecasts = [];

    if (cuaca is List) {
      for (final day in cuaca) {
        if (day is List) {
          forecasts.addAll(day);
        }
      }
    }

    if (forecasts.isEmpty) {
      return const Center(
        child:
            Text(
          'Prakiraan tidak tersedia.',
        ),
      );
    }

    return ListView.builder(
      padding:
          const EdgeInsets.all(
        16,
      ),

      itemCount:
          forecasts.length,

      itemBuilder:
          (
        context,
        index,
      ) {
        final forecast =
            forecasts[index];

        final time =
            forecast[
                    'local_datetime'] ??
                '--';

        final temperature =
            forecast['t']
                    ?.toString() ??
                '--';

        final humidity =
            forecast['hu']
                    ?.toString() ??
                '--';

        final weather =
            forecast[
                    'weather_desc'] ??
                'Tidak tersedia';

        return Card(
          margin:
              const EdgeInsets.only(
            bottom: 12,
          ),

          child:
              Padding(
            padding:
                const EdgeInsets.all(
              16,
            ),

            child:
                Row(
              children: [
                _getWeatherIcon(
                  weather,
                ),

                const SizedBox(
                  width: 16,
                ),

                Expanded(
                  child:
                      Column(
                    crossAxisAlignment:
                        CrossAxisAlignment
                            .start,

                    children: [
                      Text(
                        time,

                        style:
                            const TextStyle(
                          fontWeight:
                              FontWeight.bold,
                        ),
                      ),

                      const SizedBox(
                        height: 6,
                      ),

                      Text(
                        weather,
                      ),

                      const SizedBox(
                        height: 6,
                      ),

                      Text(
                        '$temperature°C  |  '
                        'Kelembapan '
                        '$humidity%',
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _getWeatherIcon(
    String weather,
  ) {
    final text =
        weather.toLowerCase();

    if (text.contains('petir') ||
        text.contains('badai')) {
      return const Icon(
        Icons.thunderstorm,
        size: 42,
      );
    }

    if (text.contains('hujan')) {
      return const Icon(
        Icons.water_drop,
        size: 42,
      );
    }

    if (text.contains('cerah')) {
      return const Icon(
        Icons.wb_sunny,
        size: 42,
      );
    }

    if (text.contains('berawan')) {
      return const Icon(
        Icons.cloud,
        size: 42,
      );
    }

    return const Icon(
      Icons.cloud_queue,
      size: 42,
    );
  }
}