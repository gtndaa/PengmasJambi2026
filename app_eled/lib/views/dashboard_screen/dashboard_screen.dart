import 'package:flutter/material.dart';

import '../location_screen/location_screen.dart';
import '../../services/api_service.dart';
import '../../services/location_storage_service.dart';
import '../../services/bmkg_service.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../../consts/colors.dart';

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
      backgroundColor: AppColors.background,
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

                _buildCurrentWeatherCard(),

                const SizedBox(
                  height: 24,
                ),

                _buildModernForecastSection(),
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
  // CUACA SAAT INI
  // =====================================================

  Widget _buildCurrentWeatherCard() {
    if (isLoadingWeather) {
      return Container(
        width: double.infinity,
        padding: const EdgeInsets.all(40),
        decoration: BoxDecoration(
          color: const Color(0xFFEAF4FF),
          borderRadius: BorderRadius.circular(24),
        ),
        child: const Center(
          child: CircularProgressIndicator(),
        ),
      );
    }

    if (weatherData == null) {
      return Card(
        elevation: 2,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(20),
        ),
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            children: [
              const Icon(
                Icons.cloud_off_outlined,
                size: 70,
              ),

              const SizedBox(height: 16),

              const Text(
                'Data cuaca belum tersedia',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                ),
              ),

              if (weatherError.isNotEmpty) ...[
                const SizedBox(height: 10),
                Text(
                  weatherError,
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    color: Colors.red.shade700,
                  ),
                ),
              ],

              const SizedBox(height: 16),

              ElevatedButton.icon(
                onPressed: _loadWeather,
                icon: const Icon(Icons.refresh),
                label: const Text(
                  'Muat Ulang Cuaca',
                ),
              ),
            ],
          ),
        ),
      );
    }

    final forecasts = _getForecasts();

    if (forecasts.isEmpty) {
      return const Card(
        child: Padding(
          padding: EdgeInsets.all(20),
          child: Text(
            'Data prakiraan cuaca kosong.',
          ),
        ),
      );
    }

    final current = forecasts.first;

    final temperature =
        current['t']?.toString() ?? '--';

    final weather =
        current['weather_desc'] ??
            'Tidak tersedia';

    final humidity =
        current['hu']?.toString() ?? '--';

    final wind =
        current['ws']?.toString() ?? '--';
    
        final cloud = current['tcc']?.toString() ?? '--';
    final gradient = AppColors.weatherGradient(weather);

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.fromLTRB(22, 14, 22, 22),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          colors: gradient,
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
        ),
        borderRadius: BorderRadius.circular(32),
        boxShadow: [
          BoxShadow(
            color: gradient.first.withValues(alpha: 0.35),
            blurRadius: 24,
            offset: const Offset(0, 12),
          ),
        ],
      ),
      child: Column(
        children: [
          Row(
            children: [
              const Icon(Icons.location_on_outlined,
                  color: Colors.white, size: 18),
              const SizedBox(width: 6),
              Expanded(
                child: Text(
                  '$selectedVillage, $selectedRegency',
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                  style: const TextStyle(
                    color: Colors.white,
                    fontWeight: FontWeight.w600,
                  ),
                ),
              ),
              IconButton(
                onPressed: _updateLocation,
                tooltip: 'Ubah lokasi',
                icon: const Icon(Icons.edit_location_alt_outlined,
                    color: Colors.white, size: 20),
              ),
            ],
          ),
          const SizedBox(height: 4),
          Icon(
            _getWeatherIconData(weather),
            size: 84,
            color: weather.toLowerCase().contains('cerah')
                ? Colors.amber.shade300
                : Colors.white,
          ),
          const SizedBox(height: 4),
          Text(
            '$temperature°',
            style: const TextStyle(
              fontSize: 88,
              fontWeight: FontWeight.w200,
              color: Colors.white,
              height: 1.1,
            ),
          ),
          Container(
            padding:
                const EdgeInsets.symmetric(horizontal: 14, vertical: 6),
            decoration: BoxDecoration(
              color: Colors.white.withValues(alpha: 0.22),
              borderRadius: BorderRadius.circular(20),
            ),
            child: Text(
              weather,
              style: const TextStyle(
                color: Colors.white,
                fontWeight: FontWeight.w500,
              ),
            ),
          ),
          const SizedBox(height: 22),
          Container(
            padding: const EdgeInsets.symmetric(vertical: 14),
            decoration: BoxDecoration(
              color: Colors.white.withValues(alpha: 0.22),
              borderRadius: BorderRadius.circular(20),
            ),
            child: Row(
              children: [
                Expanded(
                  child: _buildCurrentWeatherInfo(
                    icon: Icons.water_drop_outlined,
                    label: 'Kelembapan',
                    value: '$humidity%',
                    iconColor: Colors.white,
                  ),
                ),
                Expanded(
                  child: _buildCurrentWeatherInfo(
                    icon: Icons.air,
                    label: 'Angin',
                    value: '$wind km/j',
                    iconColor: Colors.white,
                  ),
                ),
                Expanded(
                  child: _buildCurrentWeatherInfo(
                    icon: Icons.cloud_outlined,
                    label: 'Awan',
                    value: '$cloud%',
                    iconColor: Colors.white,
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildCurrentWeatherInfo({
    required IconData icon,
    required String label,
    required String value,
    required Color iconColor,
  }) {
    return Column(
      children: [
        Icon(icon, size: 22, color: iconColor),
        const SizedBox(height: 6),
        Text(
          label,
          style: TextStyle(
            fontSize: 12,
            color: Colors.white.withValues(alpha: 0.85),
          ),
        ),
        const SizedBox(height: 3),
        Text(
          value,
          style: const TextStyle(
            fontSize: 15,
            fontWeight: FontWeight.bold,
            color: Colors.white,
          ),
        ),
      ],
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

  Widget _buildModernForecastSection() {
    final forecasts = _getForecasts();

    final upcoming = forecasts.length > 1
        ? forecasts.sublist(1)
        : <dynamic>[];

    final size = MediaQuery.sizeOf(context);
    final itemWidth = (size.width - 40) / 4; // 4 item per layar (padding 20+20)
    final listHeight = (size.height * 0.24).clamp(180.0, 260.0).toDouble();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            const Flexible(
              child: Text(
                'Prakiraan Cuaca',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: AppColors.ink,
                ),
              ),
            ),
            TextButton(
              onPressed: upcoming.isEmpty
                  ? null
                  : () => _showFullForecast(forecasts),
              style: TextButton.styleFrom(
                backgroundColor: AppColors.primary.withValues(alpha: 0.10),
                foregroundColor: AppColors.primary,
                padding: const EdgeInsets.symmetric(
                  horizontal: 14,
                  vertical: 6,
                ),
                shape: const StadiumBorder(),
              ),
              child: const Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text(
                    'Lihat Selengkapnya',
                    style: TextStyle(
                      fontSize: 13,
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                  SizedBox(width: 2),
                  Icon(Icons.chevron_right, size: 18),
                ],
              ),
            ),
          ],
        ),

        const SizedBox(height: 14),

        upcoming.isEmpty
            ? const Padding(
                padding: EdgeInsets.all(8),
                child: Text(
                  'Belum ada data prakiraan.',
                  style: TextStyle(fontSize: 14, color: Color(0xFF5C6B7A)),
                ),
              )
            : SizedBox(
                height: listHeight,
                child: ListView.builder(
                  scrollDirection: Axis.horizontal,
                  primary: false,
                  physics: const AlwaysScrollableScrollPhysics(
                    parent: BouncingScrollPhysics(),
                  ),
                  padding: EdgeInsets.zero,
                  itemExtent: itemWidth,
                  itemCount: upcoming.length > 8 ? 8 : upcoming.length,
                  itemBuilder: (context, index) {
                    final forecast = upcoming[index];
                    final temperature = forecast['t']?.toString() ?? '--';
                    final weather = forecast['weather_desc'] ?? '-';
                    final dateTime = forecast['local_datetime'] ?? '';

                    return _buildForecastStripItem(
                      time: _formatHourOnly(dateTime),
                      temp: '$temperature°',
                      weather: weather,
                    );
                  },
                ),
              ),
      ],
    );
  }

  Widget _buildForecastStripItem({
    required String time,
    required String temp,
    required String weather,
  }) {
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Text(
          time,
          style: const TextStyle(
            fontSize: 12,
            fontWeight: FontWeight.w500,
            color: AppColors.inkSoft,
          ),
        ),
        const SizedBox(height: 10),
        _getColoredWeatherIcon(weather, size: 28),
        const SizedBox(height: 10),
        Text(
          temp,
          style: const TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.w600,
            color: AppColors.ink,
          ),
        ),
      ],
    );
  }

  Widget _getColoredWeatherIcon(
    String weather, {
    double size = 32,
  }) {
    final text = weather.toLowerCase();
    
    IconData icon;
    Color color;

    if (text.contains('petir') || text.contains('badai')) {
      icon = Icons.thunderstorm;
      color = Colors.deepPurple;
    } else if (text.contains('hujan')) {
      icon = Icons.umbrella;
      color = Colors.blue;
    } else if (text.contains('cerah')) {
      icon = Icons.wb_sunny;
      color = Colors.amber;
    } else if (text.contains('berawan')) {
      icon = Icons.cloud;
      color = const Color.fromARGB(255, 55, 115, 255);
    } else {
      icon = Icons.cloud_outlined;
      color = const Color.fromARGB(255, 55, 115, 255);
    }

    return Icon(
      icon,
      size: size,
      color: color,
    );
  }

  String _formatHourOnly(String dateTime) {
    if (dateTime.isEmpty) return '--';

    if (dateTime.contains(' ')) {
      final timePart = dateTime.split(' ').last;
      return timePart.length >= 5 ? timePart.substring(0, 5) : timePart;
    }

    return dateTime.length >= 5 ? dateTime.substring(0, 5) : dateTime;
  }

  // =====================================================
  // FULL FORECAST
  // =====================================================

  void _showFullForecast(List<dynamic> forecasts) {
    showModalBottomSheet(
      context: context,
      isScrollControlled: true,
      backgroundColor: AppColors.background,
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(28)),
      ),
      builder: (context) {
        return SafeArea(
          child: SizedBox(
            height: MediaQuery.of(context).size.height * 0.8,
            child: Column(
              children: [
                const SizedBox(height: 10),
                Container(
                  width: 44,
                  height: 5,
                  decoration: BoxDecoration(
                    color: const Color(0xFFD5DCE5),
                    borderRadius: BorderRadius.circular(3),
                  ),
                ),
                const Padding(
                  padding: EdgeInsets.fromLTRB(22, 16, 22, 8),
                  child: Align(
                    alignment: Alignment.centerLeft,
                    child: Text(
                      'Prakiraan Cuaca',
                      style: TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.bold,
                        color: AppColors.ink,
                      ),
                    ),
                  ),
                ),
                Expanded(
                  child: ListView.separated(
                    padding: const EdgeInsets.fromLTRB(18, 8, 18, 20),
                    itemCount: forecasts.length,
                    separatorBuilder: (context, index) =>
                        const SizedBox(height: 10),
                    itemBuilder: (context, index) {
                      final forecast = forecasts[index];
                      final weather =
                          (forecast['weather_desc'] ?? '-').toString();
                      final temperature = forecast['t']?.toString() ?? '--';
                      final humidity = forecast['hu']?.toString() ?? '--';
                      final raw =
                          (forecast['local_datetime'] ?? '-').toString();
                      final dateTime =
                          raw.length >= 16 ? raw.substring(0, 16) : raw;
                      final tint = AppColors.weatherGradient(weather).first;

                      return Container(
                        padding: const EdgeInsets.all(14),
                        decoration: BoxDecoration(
                          color: Colors.white,
                          borderRadius: BorderRadius.circular(20),
                          border: Border.all(color: const Color(0xFFE8EDF3)),
                        ),
                        child: Row(
                          children: [
                            Container(
                              width: 52,
                              height: 52,
                              decoration: BoxDecoration(
                                color: tint.withValues(alpha: 0.12),
                                borderRadius: BorderRadius.circular(16),
                              ),
                              child: Center(
                                child: _getColoredWeatherIcon(weather, size: 30),
                              ),
                            ),
                            const SizedBox(width: 14),
                            Expanded(
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Text(
                                    weather,
                                    style: const TextStyle(
                                      fontSize: 15,
                                      fontWeight: FontWeight.w600,
                                      color: AppColors.ink,
                                    ),
                                  ),
                                  const SizedBox(height: 3),
                                  Text(
                                    dateTime,
                                    style: const TextStyle(
                                      fontSize: 12,
                                      color: AppColors.inkSoft,
                                    ),
                                  ),
                                  const SizedBox(height: 6),
                                  Container(
                                    padding: const EdgeInsets.symmetric(
                                      horizontal: 8,
                                      vertical: 3,
                                    ),
                                    decoration: BoxDecoration(
                                      color: AppColors.primary
                                          .withValues(alpha: 0.08),
                                      borderRadius: BorderRadius.circular(10),
                                    ),
                                    child: Row(
                                      mainAxisSize: MainAxisSize.min,
                                      children: [
                                        const Icon(
                                          Icons.water_drop_outlined,
                                          size: 13,
                                          color: AppColors.primary,
                                        ),
                                        const SizedBox(width: 4),
                                        Text(
                                          'Kelembapan $humidity%',
                                          style: const TextStyle(
                                            fontSize: 11,
                                            color: AppColors.primary,
                                          ),
                                        ),
                                      ],
                                    ),
                                  ),
                                ],
                              ),
                            ),
                            Text(
                              '$temperature°C',
                              style: const TextStyle(
                                fontSize: 22,
                                fontWeight: FontWeight.w600,
                                color: AppColors.ink,
                              ),
                            ),
                          ],
                        ),
                      );
                    },
                  ),
                ),
              ],
            ),
          ),
        );
      },
    );
  }
  
  // =====================================================
  // ICON CUACA
  // =====================================================

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
