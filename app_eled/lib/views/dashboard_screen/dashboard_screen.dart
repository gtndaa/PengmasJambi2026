import 'package:flutter/material.dart';

import '../location_screen/location_screen.dart';
import '../../services/api_service.dart';
import '../../models/user_model.dart';
import '../../services/location_storage_service.dart';
import '../../services/bmkg_service.dart';
import 'package:shared_preferences/shared_preferences.dart';

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({
    super.key,
  });

  @override
  State<DashboardScreen> createState() =>
      _DashboardScreenState();
}

class _DashboardScreenState
    extends State<DashboardScreen> {

  // =====================================================
  // SERVICE BMKG
  // =====================================================

  final BmkgService bmkgService =
      BmkgService();

  // =====================================================
  // DATA USER
  // =====================================================

  String userName = 'User';

  final ApiService apiService = ApiService();
  // =====================================================
  // DATA LOKASI
  // =====================================================

  String selectedProvince =
      'Lokasi belum dipilih';

  String selectedRegency = '';

  String selectedDistrict = '';

  String selectedVillage = '';

  String selectedAdm4 = '';

  // =====================================================
  // STATUS LOKASI
  // =====================================================

  bool isLoadingLocation = true;

  // =====================================================
  // DATA CUACA
  // =====================================================

  Map<String, dynamic>? weatherData;

  bool isLoadingWeather = false;

  String weatherError = '';

  // =====================================================
  // INIT
  // =====================================================

  @override
  void initState() {
    super.initState();
    _loadUser();
    _loadSavedLocation();
  }

  Future<void> _loadUser() async {
  try {
    final prefs =
        await SharedPreferences.getInstance();

    final savedName =
        prefs.getString(
      'user_name',
    );

    if (!mounted) return;

    if (savedName != null &&
        savedName.trim().isNotEmpty) {
      setState(() {
        userName =
            savedName.trim();
      });

            debugPrint(
        'NAMA USER DASHBOARD: $userName',
      );
    }
  } catch (e) {
    debugPrint(
      'Gagal memuat nama user: $e',
    );
  }
}

  // =====================================================
  // LOAD LOKASI YANG TERSIMPAN
  // =====================================================

  Future<void> _loadSavedLocation() async {
    try {
      final location =
          await LocationStorageService
              .getLocation();

      if (!mounted) return;

      if (location != null) {
        setState(() {
          selectedProvince =
              location['province'] ?? '';

          selectedRegency =
              location['regency'] ?? '';

          selectedDistrict =
              location['district'] ?? '';

          selectedVillage =
              location['village'] ?? '';

          selectedAdm4 =
              location['adm4'] ?? '';
        });

        debugPrint(
          '================================',
        );

        debugPrint(
          'LOKASI TERSIMPAN DITEMUKAN',
        );

        debugPrint(
          'ADM4: $selectedAdm4',
        );

        debugPrint(
          '================================',
        );

        // Ambil data cuaca setelah lokasi berhasil dimuat
        if (selectedAdm4.isNotEmpty) {
          await _loadWeather();
        }
      }

      if (!mounted) return;

      setState(() {
        isLoadingLocation = false;
      });
    } catch (e) {
      if (!mounted) return;

      setState(() {
        isLoadingLocation = false;

        weatherError =
            'Gagal memuat lokasi: $e';
      });

      debugPrint(
        'GAGAL MEMUAT LOKASI: $e',
      );
    }
  }

  // =====================================================
  // LOAD DATA CUACA BMKG
  // =====================================================

  Future<void> _loadWeather() async {
    if (selectedAdm4.isEmpty) {
      return;
    }

    try {
      if (!mounted) return;

      setState(() {
        isLoadingWeather = true;

        weatherError = '';
      });

      debugPrint(
        '================================',
      );

      debugPrint(
        'REQUEST DATA CUACA BMKG',
      );

      debugPrint(
        'ADM4: $selectedAdm4',
      );

      debugPrint(
        '================================',
      );

      final result =
          await bmkgService
              .getWeatherData(
            selectedAdm4,
          );

      if (!mounted) return;

      setState(() {
        weatherData = result;

        isLoadingWeather = false;
      });

      debugPrint(
        '================================',
      );

      debugPrint(
        'DATA CUACA BERHASIL DITERIMA',
      );

      debugPrint(
        '$result',
      );

      debugPrint(
        '================================',
      );
    } catch (e) {
      if (!mounted) return;

      setState(() {
        isLoadingWeather = false;

        weatherError =
            'Gagal mengambil data cuaca:\n$e';
      });

      debugPrint(
        '================================',
      );

      debugPrint(
        'ERROR DATA CUACA',
      );

      debugPrint(
        '$e',
      );

      debugPrint(
        '================================',
      );
    }
  }

  // =====================================================
  // UPDATE LOKASI
  // =====================================================

  Future<void> _updateLocation() async {
    final result =
        await Navigator.push(
      context,

      MaterialPageRoute(
        builder: (context) =>
            const LocationScreen(),
      ),
    );

    if (!mounted) return;

    if (result != null &&
        result is Map<String, String>) {

      setState(() {
        selectedProvince =
            result['province'] ?? '';

        selectedRegency =
            result['regency'] ?? '';

        selectedDistrict =
            result['district'] ?? '';

        selectedVillage =
            result['village'] ?? '';

        selectedAdm4 =
            result['adm4'] ?? '';

        // Hapus data cuaca lama
        weatherData = null;

        weatherError = '';
      });

      debugPrint(
        '================================',
      );

      debugPrint(
        'LOKASI BERHASIL DIPERBARUI',
      );

      debugPrint(
        'PROVINSI: $selectedProvince',
      );

      debugPrint(
        'KABUPATEN/KOTA: $selectedRegency',
      );

      debugPrint(
        'KECAMATAN: $selectedDistrict',
      );

      debugPrint(
        'KELURAHAN/DESA: $selectedVillage',
      );

      debugPrint(
        'ADM4: $selectedAdm4',
      );

      debugPrint(
        '================================',
      );

      // Ambil cuaca dari lokasi baru
      await _loadWeather();
    }
  }

  // =====================================================
  // BUILD
  // =====================================================

  @override
  Widget build(
    BuildContext context,
  ) {
    if (isLoadingLocation) {
      return const Scaffold(
        body: Center(
          child:
              CircularProgressIndicator(),
        ),
      );
    }

    return Scaffold(
      body: SafeArea(
        child: RefreshIndicator(
          onRefresh: () async {
            await _loadSavedLocation();

            if (selectedAdm4.isNotEmpty) {
              await _loadWeather();
            }
          },

          child: SingleChildScrollView(
            physics:
                const AlwaysScrollableScrollPhysics(),

            padding:
                const EdgeInsets.all(20),

            child: Column(
              crossAxisAlignment:
                  CrossAxisAlignment.start,

              children: [
                _buildHeader(),

                const SizedBox(
                  height: 24,
                ),

                _buildLocationCard(),

                const SizedBox(
                  height: 20,
                ),

                _buildCurrentWeatherCard(),

                const SizedBox(
                  height: 24,
                ),

                _buildForecastSection(),
              ],
            ),
          ),
        ),
      ),
    );
  }

  // =====================================================
  // HEADER
  // =====================================================

  Widget _buildHeader() {
    return Row(
      mainAxisAlignment:
          MainAxisAlignment.spaceBetween,

      children: [
        Column(
          crossAxisAlignment:
              CrossAxisAlignment.start,

          children: [
            Text(
              'Hai, $userName!',

              style: const TextStyle(
                fontSize: 28,

                fontWeight:
                    FontWeight.bold,
              ),
            ),

            const SizedBox(
              height: 6,
            ),

            Text(
              'Selamat datang kembali',

              style: TextStyle(
                color:
                    Colors.grey.shade600,

                fontSize: 14,
              ),
            ),
          ],
        ),

        CircleAvatar(
          radius: 24,

          child: Text(
            userName.isEmpty
                ? "U"
                : userName[0].toUpperCase(),
  
            style:
                const TextStyle(
              fontSize: 20,

              fontWeight:
                  FontWeight.bold,
            ),
          ),
        ),
      ],
    );
  }

  // =====================================================
  // LOCATION CARD
  // =====================================================
  Widget _buildLocationCard() {
    final hasLocation =
        selectedAdm4.isNotEmpty;

    return Card(
      elevation: 2,

      shape:
          RoundedRectangleBorder(
        borderRadius:
            BorderRadius.circular(
          20,
        ),
      ),

      child: Padding(
        padding:
            const EdgeInsets.all(18),

        child: Column(
          crossAxisAlignment:
              CrossAxisAlignment.start,

          children: [
            const Row(
              children: [
                Icon(
                  Icons.location_on,
                  size: 22,
                ),

                SizedBox(
                  width: 8,
                ),

                Text(
                  'Lokasi Anda',
                  style: TextStyle(
                    fontSize: 18,
                    fontWeight:
                        FontWeight.bold,
                  ),
                ),
              ],
            ),

            const SizedBox(
              height: 12,
            ),

            if (!hasLocation)
              const Text(
                'Belum ada lokasi yang dipilih.',
              )
            else
              Column(
                crossAxisAlignment:
                    CrossAxisAlignment.start,

                children: [
                  Text(
                    selectedVillage,

                    style:
                        const TextStyle(
                      fontSize: 20,

                      fontWeight:
                          FontWeight.bold,
                    ),
                  ),

                  const SizedBox(
                    height: 4,
                  ),

                  Text(
                    '$selectedDistrict, '
                    '$selectedRegency',

                    style:
                        TextStyle(
                      color:
                          Colors.grey.shade700,
                    ),
                  ),

                  const SizedBox(
                    height: 4,
                  ),

                  Text(
                    selectedProvince,

                    style:
                        TextStyle(
                      color:
                          Colors.grey.shade600,

                      fontSize: 13,
                    ),
                  ),
                ],
              ),

            const SizedBox(
              height: 16,
            ),

            SizedBox(
              width:
                  double.infinity,

              child:
                  OutlinedButton.icon(
                onPressed:
                    _updateLocation,

                icon: const Icon(
                  Icons.location_on_outlined,
                ),

                label: const Text(
                  'Ubah Lokasi',
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // CUACA SAAT INI
  // =====================================================

  Widget _buildCurrentWeatherCard() {
    if (isLoadingWeather) {
      return Card(
        elevation: 2,

        shape:
            RoundedRectangleBorder(
          borderRadius:
              BorderRadius.circular(
            20,
          ),
        ),

        child: const Padding(
          padding:
              EdgeInsets.all(40),

          child: Center(
            child:
                CircularProgressIndicator(),
          ),
        ),
      );
    }

    if (weatherData == null) {
      return Card(
        elevation: 2,

        shape:
            RoundedRectangleBorder(
          borderRadius:
              BorderRadius.circular(
            20,
          ),
        ),

        child: Padding(
          padding:
              const EdgeInsets.all(20),

          child: Column(
            children: [
              const Icon(
                Icons.cloud_off_outlined,

                size: 70,
              ),

              const SizedBox(
                height: 16,
              ),

              const Text(
                'Data cuaca belum tersedia',

                style:
                    TextStyle(
                  fontSize: 18,

                  fontWeight:
                      FontWeight.bold,
                ),
              ),

              if (weatherError
                  .isNotEmpty) ...[
                const SizedBox(
                  height: 10,
                ),

                Text(
                  weatherError,

                  textAlign:
                      TextAlign.center,

                  style:
                      TextStyle(
                    color:
                        Colors.red.shade700,
                  ),
                ),
              ],

              const SizedBox(
                height: 16,
              ),

              ElevatedButton.icon(
                onPressed:
                    _loadWeather,

                icon: const Icon(
                  Icons.refresh,
                ),

                label: const Text(
                  'Muat Ulang Cuaca',
                ),
              ),
            ],
          ),
        ),
      );
    }

    final forecasts =
        _getForecasts();

    if (forecasts.isEmpty) {
      return const Card(
        child: Padding(
          padding:
              EdgeInsets.all(20),

          child: Text(
            'Data prakiraan cuaca kosong.',
          ),
        ),
      );
    }

    final current =
        forecasts.first;

    final temperature =
        current['t']?.toString() ??
            '--';

    final weather =
        current['weather_desc'] ??
            'Tidak tersedia';

    final humidity =
        current['hu']?.toString() ??
            '--';

    final wind =
        current['ws']?.toString() ??
            '--';

    final visibility =
        current['vs_text'] ??
            '--';

    return Card(
      elevation: 2,

      shape:
          RoundedRectangleBorder(
        borderRadius:
            BorderRadius.circular(
          20,
        ),
      ),

      child: Padding(
        padding:
            const EdgeInsets.all(20),

        child: Column(
          crossAxisAlignment:
              CrossAxisAlignment.start,

          children: [
            const Text(
              'Cuaca Saat Ini',

              style:
                  TextStyle(
                fontSize: 20,

                fontWeight:
                    FontWeight.bold,
              ),
            ),

            const SizedBox(
              height: 20,
            ),

            Row(
              mainAxisAlignment:
                  MainAxisAlignment
                      .spaceBetween,

              children: [
                Column(
                  crossAxisAlignment:
                      CrossAxisAlignment
                          .start,

                  children: [
                    Text(
                      '$temperature°C',

                      style:
                          const TextStyle(
                        fontSize: 48,

                        fontWeight:
                            FontWeight.bold,
                      ),
                    ),

                    Text(
                      weather,

                      style:
                          TextStyle(
                        color:
                            Colors.grey.shade600,

                        fontSize: 16,
                      ),
                    ),
                  ],
                ),

                _getWeatherIcon(
                  weather,

                  size: 70,
                ),
              ],
            ),

            const SizedBox(
              height: 24,
            ),

            const Divider(),

            const SizedBox(
              height: 12,
            ),

            Row(
              mainAxisAlignment:
                  MainAxisAlignment
                      .spaceAround,

              children: [
                _buildWeatherInfo(
                  Icons.water_drop_outlined,

                  'Kelembapan',

                  '$humidity%',
                ),

                _buildWeatherInfo(
                  Icons.air,

                  'Angin',

                  '$wind km/j',
                ),

                _buildWeatherInfo(
                  Icons.visibility_outlined,

                  'Jarak Pandang',

                  visibility,
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // MENGAMBIL DATA PRAKIRAAN
  // =====================================================

  List<dynamic> _getForecasts() {
    if (weatherData == null) {
      return [];
    }

    final data =
        weatherData!['data'];

    if (data is! List ||
        data.isEmpty) {
      return [];
    }

    final cuaca =
        data[0]['cuaca'];

    if (cuaca is! List) {
      return [];
    }

    final List<dynamic> forecasts =
        [];

    for (final day in cuaca) {
      if (day is List) {
        forecasts.addAll(day);
      }
    }

    return forecasts;
  }

  // =====================================================
  // PRAKIRAAN CUACA
  // =====================================================

  Widget _buildForecastSection() {
    final forecasts =
        _getForecasts();

    final upcoming =
        forecasts.length > 1
            ? forecasts.sublist(1)
            : [];

    return Column(
      crossAxisAlignment:
          CrossAxisAlignment.start,

      children: [
        Row(
          mainAxisAlignment:
              MainAxisAlignment
                  .spaceBetween,

          children: [
            const Text(
              'Prakiraan Cuaca',

              style:
                  TextStyle(
                fontSize: 20,

                fontWeight:
                    FontWeight.bold,
              ),
            ),

            TextButton(
              onPressed:
                  upcoming.isEmpty
                      ? null
                      : () {
                          _showFullForecast(
                            forecasts,
                          );
                        },

              child: const Text(
                'Lihat Selengkapnya',
              ),
            ),
          ],
        ),

        const SizedBox(
          height: 12,
        ),

        if (upcoming.isEmpty)
          const Text(
            'Belum ada data prakiraan.',
          )
        else
          SizedBox(
            height: 150,

            child: ListView.builder(
              scrollDirection:
                  Axis.horizontal,

              itemCount:
                  upcoming.length > 8
                      ? 8
                      : upcoming.length,

              itemBuilder:
                  (
                context,
                index,
              ) {
                final forecast =
                    upcoming[index];

                final weather =
                    forecast[
                            'weather_desc'] ??
                        '-';

                final temperature =
                    forecast['t']
                            ?.toString() ??
                        '--';

                final dateTime =
                    forecast[
                            'local_datetime'] ??
                        '';

                return _buildForecastCard(
                  dateTime,

                  '$temperature°C',

                  _getWeatherIconData(
                    weather,
                  ),
                );
              },
            ),
          ),
      ],
    );
  }

  // =====================================================
  // FULL FORECAST
  // =====================================================

  void _showFullForecast(
    List<dynamic> forecasts,
  ) {
    showModalBottomSheet(
      context: context,

      isScrollControlled:
          true,

      builder:
          (context) {
        return SafeArea(
          child: SizedBox(
            height:
                MediaQuery.of(
                  context,
                ).size.height *
                    0.8,

            child: ListView.builder(
              padding:
                  const EdgeInsets.all(
                20,
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

                final weather =
                    forecast[
                            'weather_desc'] ??
                        '-';

                final temperature =
                    forecast['t']
                            ?.toString() ??
                        '--';

                final humidity =
                    forecast['hu']
                            ?.toString() ??
                        '--';

                final dateTime =
                    forecast[
                            'local_datetime'] ??
                        '-';

                return Card(
                  child: ListTile(
                    leading:
                        _getWeatherIcon(
                      weather,

                      size: 36,
                    ),

                    title: Text(
                      '$temperature°C - '
                      '$weather',
                    ),

                    subtitle: Text(
                      '$dateTime\n'
                      'Kelembapan: '
                      '$humidity%',
                    ),
                  ),
                );
              },
            ),
          ),
        );
      },
    );
  }

  // =====================================================
  // FORECAST CARD
  // =====================================================

  Widget _buildForecastCard(
    String time,

    String temperature,

    IconData icon,
  ) {
    String displayTime =
        time;

    if (time.contains(' ')) {
      displayTime =
          time.split(' ').last;
    }

    return Container(
      width: 125,

      margin:
          const EdgeInsets.only(
        right: 12,
      ),

      child: Card(
        elevation: 2,

        child: Padding(
          padding:
              const EdgeInsets.all(
            12,
          ),

          child: Column(
            mainAxisAlignment:
                MainAxisAlignment
                    .center,

            children: [
              Text(
                displayTime,

                style:
                    const TextStyle(
                  fontWeight:
                      FontWeight.bold,
                ),
              ),

              const SizedBox(
                height: 12,
              ),

              Icon(
                icon,

                size: 36,
              ),

              const SizedBox(
                height: 8,
              ),

              Text(
                temperature,

                style:
                    const TextStyle(
                  fontSize: 18,

                  fontWeight:
                      FontWeight.bold,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // =====================================================
  // WEATHER INFO
  // =====================================================

  Widget _buildWeatherInfo(
    IconData icon,

    String label,

    String value,
  ) {
    return Column(
      children: [
        Icon(
          icon,

          size: 24,
        ),

        const SizedBox(
          height: 6,
        ),

        Text(
          label,

          style:
              const TextStyle(
            fontSize: 11,
          ),
        ),

        const SizedBox(
          height: 4,
        ),

        Text(
          value,

          style:
              const TextStyle(
            fontWeight:
                FontWeight.bold,
          ),
        ),
      ],
    );
  }

  // =====================================================
  // ICON CUACA
  // =====================================================

  Widget _getWeatherIcon(
    String weather, {

    double size = 50,
  }) {
    return Icon(
      _getWeatherIconData(
        weather,
      ),

      size: size,
    );
  }

  IconData _getWeatherIconData(
    String weather,
  ) {
    final text =
        weather.toLowerCase();

    if (text.contains('petir') ||
        text.contains('badai')) {
      return Icons.thunderstorm;
    }

    if (text.contains('hujan')) {
      return Icons.umbrella;
    }

    if (text.contains('cerah')) {
      return Icons.wb_sunny;
    }

    if (text.contains('berawan')) {
      return Icons.cloud;
    }

    return Icons.cloud_outlined;
  }
}
