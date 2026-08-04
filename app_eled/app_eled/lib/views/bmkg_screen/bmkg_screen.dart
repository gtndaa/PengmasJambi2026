import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../services/wilayah_local_service.dart';
import '../../services/bmkg_service.dart';

class BmkgScreen extends StatefulWidget {
  const BmkgScreen({
    super.key,
  });

  @override
  State<BmkgScreen> createState() => _BmkgScreenState();
}

class _BmkgScreenState extends State<BmkgScreen> {
  // =====================================================
  // SERVICES
  // =====================================================

  final WilayahLocalService wilayahService =
      WilayahLocalService();

  final BmkgService bmkgService =
      BmkgService();

  // =====================================================
  // DATA WILAYAH
  // =====================================================

  List<WilayahData> provinces = [];

  List<WilayahData> regencies = [];

  List<WilayahData> districts = [];

  List<WilayahData> villages = [];

  // =====================================================
  // LOKASI TERPILIH
  // =====================================================

  WilayahData? selectedProvince;

  WilayahData? selectedRegency;

  WilayahData? selectedDistrict;

  WilayahData? selectedVillage;

  // =====================================================
  // DATA CUACA
  // =====================================================

  Map<String, dynamic>? weatherData;

  // =====================================================
  // STATUS
  // =====================================================

  bool isLoadingDatabase = true;

  bool isLoadingWeather = false;

  bool isLoadingSavedData = true;

  String errorMessage = '';

  // =====================================================
  // HALAMAN
  // =====================================================

  bool showWeatherPage = false;

  // =====================================================
  // INIT
  // =====================================================

  @override
  void initState() {
    super.initState();

    initializeScreen();
  }

  // =====================================================
  // INITIALIZE SCREEN
  // =====================================================

  Future<void> initializeScreen() async {
    await loadDatabase();

    await loadSavedLocation();

    if (!mounted) return;

    setState(() {
      isLoadingSavedData = false;
    });
  }

  // =====================================================
  // LOAD DATABASE WILAYAH
  // =====================================================

  Future<void> loadDatabase() async {
    try {
      setState(() {
        isLoadingDatabase = true;

        errorMessage = '';
      });

      await wilayahService.loadDatabase();

      final data =
          wilayahService.getProvinces();

      if (!mounted) return;

      setState(() {
        provinces = data;

        isLoadingDatabase = false;
      });

      print('================================');
      print('DATABASE WILAYAH BERHASIL DIMUAT');
      print('JUMLAH PROVINSI: ${provinces.length}');
      print('================================');
    } catch (e) {
      if (!mounted) return;

      setState(() {
        isLoadingDatabase = false;

        errorMessage =
            'Gagal membaca database wilayah:\n$e';
      });

      print('================================');
      print('ERROR DATABASE WILAYAH');
      print(e);
      print('================================');
    }
  }

  // =====================================================
  // LOAD DATA LOKASI YANG TERSIMPAN
  // =====================================================

  Future<void> loadSavedLocation() async {
    try {
      final prefs =
          await SharedPreferences.getInstance();

      final provinceCode =
          prefs.getString('selectedProvinceCode');

      final regencyCode =
          prefs.getString('selectedRegencyCode');

      final districtCode =
          prefs.getString('selectedDistrictCode');

      final villageCode =
          prefs.getString('selectedVillageCode');

      // Jika belum ada data tersimpan
      if (provinceCode == null) {
        return;
      }

      // =====================================================
      // PROVINSI
      // =====================================================

      final province =
          provinces.firstWhere(
        (item) => item.code == provinceCode,
      );

      final regenciesData =
          wilayahService.getRegencies(
        province.code,
      );

      // =====================================================
      // KABUPATEN / KOTA
      // =====================================================

      WilayahData? regency;

      if (regencyCode != null) {
        try {
          regency =
              regenciesData.firstWhere(
            (item) =>
                item.code == regencyCode,
          );
        } catch (_) {
          regency = null;
        }
      }

      List<WilayahData> districtsData = [];

      if (regency != null) {
        districtsData =
            wilayahService.getDistricts(
          regency.code,
        );
      }

      // =====================================================
      // KECAMATAN
      // =====================================================

      WilayahData? district;

      if (districtCode != null) {
        try {
          district =
              districtsData.firstWhere(
            (item) =>
                item.code == districtCode,
          );
        } catch (_) {
          district = null;
        }
      }

      List<WilayahData> villagesData = [];

      if (district != null) {
        villagesData =
            wilayahService.getVillages(
          district.code,
        );
      }

      // =====================================================
      // KELURAHAN / DESA
      // =====================================================

      WilayahData? village;

      if (villageCode != null) {
        try {
          village =
              villagesData.firstWhere(
            (item) =>
                item.code == villageCode,
          );
        } catch (_) {
          village = null;
        }
      }

      if (!mounted) return;

      setState(() {
        selectedProvince = province;

        selectedRegency = regency;

        selectedDistrict = district;

        selectedVillage = village;

        regencies = regenciesData;

        districts = districtsData;

        villages = villagesData;
      });

      // =====================================================
      // LOAD WEATHER YANG TERSIMPAN
      // =====================================================

      final savedWeather =
          prefs.getString('weatherData');

      final savedShowWeather =
          prefs.getBool('showWeatherPage') ??
              false;

      if (savedWeather != null) {
        try {
          final decodedWeather =
              jsonDecode(savedWeather);

          if (!mounted) return;

          setState(() {
            weatherData =
                Map<String, dynamic>.from(
              decodedWeather,
            );

            showWeatherPage =
                savedShowWeather;
          });
        } catch (e) {
          print(
            'Gagal membaca data cuaca tersimpan: $e',
          );
        }
      }

      print('================================');
      print('DATA LOKASI BERHASIL DIPULIHKAN');
      print('PROVINSI: ${province.name}');
      print('KAB/KOTA: ${regency?.name}');
      print('KECAMATAN: ${district?.name}');
      print('DESA: ${village?.name}');
      print('================================');
    } catch (e) {
      print('================================');
      print('TIDAK ADA DATA LOKASI TERSIMPAN');
      print(e);
      print('================================');
    }
  }

  // =====================================================
  // SIMPAN DATA LOKASI
  // =====================================================

  Future<void> saveLocationData() async {
    final prefs =
        await SharedPreferences.getInstance();

    if (selectedProvince != null) {
      await prefs.setString(
        'selectedProvinceCode',
        selectedProvince!.code,
      );

      await prefs.setString(
        'selectedProvinceName',
        selectedProvince!.name,
      );
    }

    if (selectedRegency != null) {
      await prefs.setString(
        'selectedRegencyCode',
        selectedRegency!.code,
      );

      await prefs.setString(
        'selectedRegencyName',
        selectedRegency!.name,
      );
    }

    if (selectedDistrict != null) {
      await prefs.setString(
        'selectedDistrictCode',
        selectedDistrict!.code,
      );

      await prefs.setString(
        'selectedDistrictName',
        selectedDistrict!.name,
      );
    }

    if (selectedVillage != null) {
      await prefs.setString(
        'selectedVillageCode',
        selectedVillage!.code,
      );

      await prefs.setString(
        'selectedVillageName',
        selectedVillage!.name,
      );
    }

    print('DATA LOKASI BERHASIL DISIMPAN');
  }

  // =====================================================
  // SIMPAN DATA CUACA
  // =====================================================

  Future<void> saveWeatherData() async {
    if (weatherData == null) {
      return;
    }

    final prefs =
        await SharedPreferences.getInstance();

    await prefs.setString(
      'weatherData',
      jsonEncode(weatherData),
    );

    await prefs.setBool(
      'showWeatherPage',
      showWeatherPage,
    );

    print('DATA CUACA BERHASIL DISIMPAN');
  }

  // =====================================================
  // PILIH PROVINSI
  // =====================================================

  void onProvinceChanged(
    WilayahData? value,
  ) {
    if (value == null) return;

    final data =
        wilayahService.getRegencies(
      value.code,
    );

    setState(() {
      selectedProvince = value;

      selectedRegency = null;

      selectedDistrict = null;

      selectedVillage = null;

      regencies = data;

      districts = [];

      villages = [];

      weatherData = null;

      showWeatherPage = false;

      errorMessage = '';
    });

    saveLocationData();

    print('================================');
    print('PROVINSI DIPILIH');
    print('NAMA: ${value.name}');
    print('KODE: ${value.code}');
    print('================================');
  }

  // =====================================================
  // PILIH KABUPATEN / KOTA
  // =====================================================

  void onRegencyChanged(
    WilayahData? value,
  ) {
    if (value == null) return;

    final data =
        wilayahService.getDistricts(
      value.code,
    );

    setState(() {
      selectedRegency = value;

      selectedDistrict = null;

      selectedVillage = null;

      districts = data;

      villages = [];

      weatherData = null;

      showWeatherPage = false;

      errorMessage = '';
    });

    saveLocationData();

    print('================================');
    print('KABUPATEN / KOTA DIPILIH');
    print('NAMA: ${value.name}');
    print('KODE: ${value.code}');
    print('================================');
  }

  // =====================================================
  // PILIH KECAMATAN
  // =====================================================

  void onDistrictChanged(
    WilayahData? value,
  ) {
    if (value == null) return;

    final data =
        wilayahService.getVillages(
      value.code,
    );

    setState(() {
      selectedDistrict = value;

      selectedVillage = null;

      villages = data;

      weatherData = null;

      showWeatherPage = false;

      errorMessage = '';
    });

    saveLocationData();

    print('================================');
    print('KECAMATAN DIPILIH');
    print('NAMA: ${value.name}');
    print('KODE: ${value.code}');
    print('================================');
  }

  // =====================================================
  // PILIH KELURAHAN / DESA
  // =====================================================

  void onVillageChanged(
    WilayahData? value,
  ) {
    if (value == null) return;

    setState(() {
      selectedVillage = value;

      weatherData = null;

      showWeatherPage = false;

      errorMessage = '';
    });

    saveLocationData();

    print('================================');
    print('LOKASI DIPILIH');
    print(
      'PROVINSI: ${selectedProvince?.name}',
    );
    print(
      'KAB/KOTA: ${selectedRegency?.name}',
    );
    print(
      'KECAMATAN: ${selectedDistrict?.name}',
    );
    print(
      'DESA/KELURAHAN: ${selectedVillage?.name}',
    );
    print(
      'KODE ADM4 INTERNAL: ${selectedVillage?.code}',
    );
    print('================================');
  }

  // =====================================================
  // AMBIL DATA CUACA BMKG
  // =====================================================

  Future<void> loadWeather() async {
    if (selectedVillage == null) {
      setState(() {
        errorMessage =
            'Silakan pilih kelurahan/desa terlebih dahulu.';
      });

      return;
    }

    try {
      setState(() {
        isLoadingWeather = true;

        errorMessage = '';
      });

      final adm4 =
          selectedVillage!.code;

      print('================================');
      print('MENGAMBIL DATA CUACA BMKG');
      print('ADM4: $adm4');
      print('================================');

      final result =
          await bmkgService.getWeatherData(
        adm4,
      );

      if (!mounted) return;

      setState(() {
        weatherData = result;

        isLoadingWeather = false;

        showWeatherPage = true;
      });

      await saveWeatherData();

      print('================================');
      print('DATA CUACA BERHASIL DIDAPATKAN');
      print('================================');
    } catch (e) {
      if (!mounted) return;

      setState(() {
        isLoadingWeather = false;

        errorMessage =
            'Gagal mengambil data cuaca:\n$e';
      });

      print('================================');
      print('ERROR DATA CUACA');
      print(e);
      print('================================');
    }
  }

  // =====================================================
  // KEMBALI KE HALAMAN LOKASI
  // =====================================================

  void backToLocationPage() {
    setState(() {
      showWeatherPage = false;

      errorMessage = '';
    });

    saveWeatherData();
  }

  // =====================================================
  // BUILD
  // =====================================================

  @override
  Widget build(
    BuildContext context,
  ) {
    if (isLoadingDatabase ||
        isLoadingSavedData) {
      return Scaffold(
        appBar: AppBar(
          title: const Text(
            'Prakiraan Cuaca BMKG',
          ),
        ),
        body: const Center(
          child:
              CircularProgressIndicator(),
        ),
      );
    }

    if (showWeatherPage &&
        weatherData != null) {
      return _buildWeatherPage();
    }

    return _buildLocationPage();
  }

  // =====================================================
  // HALAMAN PILIH LOKASI
  // =====================================================

  Widget _buildLocationPage() {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Pilih Lokasi',
        ),
      ),

      body: SingleChildScrollView(
        padding:
            const EdgeInsets.all(16),

        child: Column(
          crossAxisAlignment:
              CrossAxisAlignment.stretch,

          children: [
            const Text(
              'PILIH LOKASI CUACA',

              style: TextStyle(
                fontSize: 24,

                fontWeight:
                    FontWeight.bold,
              ),
            ),

            const SizedBox(height: 8),

            Text(
              'Pilih lokasi secara bertingkat '
              'untuk mendapatkan prakiraan cuaca.',

              style: TextStyle(
                color:
                    Colors.grey.shade600,
              ),
            ),

            const SizedBox(height: 24),

            _buildDropdown(
              label: 'Provinsi',

              value:
                  selectedProvince,

              items: provinces,

              enabled:
                  provinces.isNotEmpty,

              onChanged:
                  onProvinceChanged,
            ),

            const SizedBox(height: 16),

            _buildDropdown(
              label:
                  'Kabupaten/Kota',

              value:
                  selectedRegency,

              items:
                  regencies,

              enabled:
                  selectedProvince !=
                          null &&
                      regencies.isNotEmpty,

              onChanged:
                  onRegencyChanged,
            ),

            const SizedBox(height: 16),

            _buildDropdown(
              label:
                  'Kecamatan',

              value:
                  selectedDistrict,

              items:
                  districts,

              enabled:
                  selectedRegency !=
                          null &&
                      districts.isNotEmpty,

              onChanged:
                  onDistrictChanged,
            ),

            const SizedBox(height: 16),

            _buildDropdown(
              label:
                  'Kelurahan/Desa',

              value:
                  selectedVillage,

              items:
                  villages,

              enabled:
                  selectedDistrict !=
                          null &&
                      villages.isNotEmpty,

              onChanged:
                  onVillageChanged,
            ),

            const SizedBox(height: 24),

            if (selectedVillage != null)
              _buildSelectedLocation(),

            const SizedBox(height: 24),

            if (errorMessage.isNotEmpty)
              _buildError(),

            const SizedBox(height: 16),

            ElevatedButton.icon(
              onPressed:
                  selectedVillage ==
                              null ||
                          isLoadingWeather
                      ? null
                      : loadWeather,

              icon:
                  isLoadingWeather
                      ? const SizedBox(
                          width: 18,
                          height: 18,
                          child:
                              CircularProgressIndicator(
                            strokeWidth: 2,
                          ),
                        )
                      : const Icon(
                          Icons.cloud,
                        ),

              label: Text(
                isLoadingWeather
                    ? 'Mengambil Data Cuaca...'
                    : 'Tampilkan Cuaca',
              ),

              style:
                  ElevatedButton.styleFrom(
                padding:
                    const EdgeInsets.symmetric(
                  vertical: 16,
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // DROPDOWN
  // =====================================================

  Widget _buildDropdown({
    required String label,

    required WilayahData? value,

    required List<WilayahData>
        items,

    required bool enabled,

    required Function(
      WilayahData?,
    ) onChanged,
  }) {
    return DropdownButtonFormField<
        WilayahData>(
      value: value,

      isExpanded: true,

      decoration:
          InputDecoration(
        labelText: label,

        border:
            const OutlineInputBorder(),
      ),

      hint: Text(
        'Pilih $label',
      ),

      items:
          items.map(
        (
          WilayahData item,
        ) {
          return DropdownMenuItem<
              WilayahData>(
            value: item,

            child: Text(
              item.name,
            ),
          );
        },
      ).toList(),

      onChanged:
          enabled
              ? onChanged
              : null,
    );
  }

  // =====================================================
  // LOKASI TERPILIH
  // =====================================================

  Widget _buildSelectedLocation() {
    return Card(
      elevation: 2,

      child: Padding(
        padding:
            const EdgeInsets.all(16),

        child: Column(
          crossAxisAlignment:
              CrossAxisAlignment.start,

          children: [
            const Text(
              'LOKASI TERPILIH',

              style: TextStyle(
                fontSize: 18,

                fontWeight:
                    FontWeight.bold,
              ),
            ),

            const Divider(),

            _buildLocationRow(
              'Provinsi',

              selectedProvince!
                  .name,
            ),

            _buildLocationRow(
              'Kabupaten/Kota',

              selectedRegency!
                  .name,
            ),

            _buildLocationRow(
              'Kecamatan',

              selectedDistrict!
                  .name,
            ),

            _buildLocationRow(
              'Kelurahan/Desa',

              selectedVillage!
                  .name,
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // LOCATION ROW
  // =====================================================

  Widget _buildLocationRow(
    String label,

    String value,
  ) {
    return Padding(
      padding:
          const EdgeInsets.symmetric(
        vertical: 4,
      ),

      child: Row(
        crossAxisAlignment:
            CrossAxisAlignment.start,

        children: [
          SizedBox(
            width: 130,

            child: Text(
              label,

              style:
                  const TextStyle(
                fontWeight:
                    FontWeight.w600,
              ),
            ),
          ),

          const Text(': '),

          Expanded(
            child: Text(
              value,
            ),
          ),
        ],
      ),
    );
  }

  // =====================================================
  // HALAMAN CUACA
  // =====================================================

  Widget _buildWeatherPage() {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Hasil Prakiraan Cuaca',
        ),

        leading:
            IconButton(
          icon:
              const Icon(
            Icons.arrow_back,
          ),

          onPressed:
              backToLocationPage,
        ),
      ),

      body:
          SingleChildScrollView(
        padding:
            const EdgeInsets.all(16),

        child: Column(
          crossAxisAlignment:
              CrossAxisAlignment
                  .stretch,

          children: [
            OutlinedButton.icon(
              onPressed:
                  backToLocationPage,

              icon:
                  const Icon(
                Icons.location_on,
              ),

              label:
                  const Text(
                'Pilih Lokasi Lain',
              ),
            ),

            const SizedBox(
              height: 20,
            ),

            _buildWeatherLocation(),

            const SizedBox(
              height: 24,
            ),

            _buildWeatherData(),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // INFORMASI LOKASI CUACA
  // =====================================================

  Widget _buildWeatherLocation() {
    return Card(
      elevation: 2,

      child: Padding(
        padding:
            const EdgeInsets.all(16),

        child: Column(
          crossAxisAlignment:
              CrossAxisAlignment
                  .start,

          children: [
            const Text(
              'LOKASI CUACA',

              style: TextStyle(
                fontSize: 18,

                fontWeight:
                    FontWeight.bold,
              ),
            ),

            const Divider(),

            _buildLocationRow(
              'Provinsi',

              selectedProvince!
                  .name,
            ),

            _buildLocationRow(
              'Kabupaten/Kota',

              selectedRegency!
                  .name,
            ),

            _buildLocationRow(
              'Kecamatan',

              selectedDistrict!
                  .name,
            ),

            _buildLocationRow(
              'Kelurahan/Desa',

              selectedVillage!
                  .name,
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // DATA CUACA
  // =====================================================

  Widget _buildWeatherData() {
    if (weatherData == null) {
      return const Center(
        child:
            CircularProgressIndicator(),
      );
    }

    final data =
        weatherData!['data'];

    if (data is! List ||
        data.isEmpty) {
      return const Card(
        child: Padding(
          padding:
              EdgeInsets.all(16),

          child: Text(
            'Data cuaca tidak tersedia.',
          ),
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
      return const Card(
        child: Padding(
          padding:
              EdgeInsets.all(16),

          child: Text(
            'Data prakiraan cuaca tidak tersedia.',
          ),
        ),
      );
    }

    final currentWeather =
        forecasts.first;

    final upcomingWeather =
        forecasts.length > 1
            ? forecasts.sublist(1)
            : <dynamic>[];

    return Column(
      crossAxisAlignment:
          CrossAxisAlignment
              .stretch,

      children: [
        _buildCurrentWeatherCard(
          currentWeather,
        ),

        const SizedBox(
          height: 24,
        ),

        if (upcomingWeather
            .isNotEmpty)
          const Text(
            'PRAKIRAAN BERIKUTNYA',

            style: TextStyle(
              fontSize: 20,

              fontWeight:
                  FontWeight.bold,
            ),
          ),

        if (upcomingWeather
            .isNotEmpty)
          const SizedBox(
            height: 12,
          ),

        if (upcomingWeather
            .isNotEmpty)
          SizedBox(
            height: 190,

            child:
                ListView.builder(
              scrollDirection:
                  Axis.horizontal,

              itemCount:
                  upcomingWeather
                      .length,

              itemBuilder:
                  (
                context,

                index,
              ) {
                return _buildUpcomingWeatherCard(
                  upcomingWeather[
                      index],
                );
              },
            ),
          ),
      ],
    );
  }

  // =====================================================
  // CUACA TERDEKAT
  // =====================================================

  Widget _buildCurrentWeatherCard(
    dynamic forecast,
  ) {
    final weather =
        forecast[
                'weather_desc'] ??
            'Tidak tersedia';

    final temperature =
        forecast['t']
                ?.toString() ??
            '-';

    final humidity =
        forecast['hu']
                ?.toString() ??
            '-';

    final windSpeed =
        forecast['ws']
                ?.toString() ??
            '-';

    final dateTime =
        forecast[
                'local_datetime'] ??
            '-';

    return Card(
      elevation: 5,

      shape:
          RoundedRectangleBorder(
        borderRadius:
            BorderRadius.circular(
          24,
        ),
      ),

      child: Padding(
        padding:
            const EdgeInsets.all(
          24,
        ),

        child: Column(
          children: [
            const Text(
              'CUACA TERDEKAT',

              style: TextStyle(
                fontSize: 18,

                fontWeight:
                    FontWeight.bold,
              ),
            ),

            const SizedBox(
              height: 6,
            ),

            Text(
              dateTime,

              style: TextStyle(
                color:
                    Colors.grey.shade600,

                fontSize: 14,
              ),
            ),

            const SizedBox(
              height: 20,
            ),

            Row(
              mainAxisAlignment:
                  MainAxisAlignment
                      .center,

              children: [
                _getWeatherIcon(
                  weather,

                  size: 80,
                ),

                const SizedBox(
                  width: 24,
                ),

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
                          const TextStyle(
                        fontSize: 18,
                      ),
                    ),
                  ],
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
                  Icons.water_drop,

                  'Kelembapan',

                  '$humidity%',
                ),

                _buildWeatherInfo(
                  Icons.air,

                  'Kecepatan Angin',

                  '$windSpeed km/jam',
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // PRAKIRAAN BERIKUTNYA
  // =====================================================

  Widget _buildUpcomingWeatherCard(
    dynamic forecast,
  ) {
    final weather =
        forecast[
                'weather_desc'] ??
            '-';

    final temperature =
        forecast['t']
                ?.toString() ??
            '-';

    final dateTime =
        forecast[
                'local_datetime'] ??
            '-';

    String time =
        dateTime;

    if (dateTime
        .contains(' ')) {
      time =
          dateTime
              .split(' ')
              .last;
    }

    return Container(
      width: 145,

      margin:
          const EdgeInsets.only(
        right: 12,
      ),

      child: Card(
        elevation: 3,

        shape:
            RoundedRectangleBorder(
          borderRadius:
              BorderRadius.circular(
            18,
          ),
        ),

        child: Padding(
          padding:
              const EdgeInsets.all(
            14,
          ),

          child: Column(
            mainAxisAlignment:
                MainAxisAlignment
                    .center,

            children: [
              Text(
                time,

                style:
                    const TextStyle(
                  fontWeight:
                      FontWeight.bold,

                  fontSize: 16,
                ),
              ),

              const SizedBox(
                height: 12,
              ),

              _getWeatherIcon(
                weather,

                size: 48,
              ),

              const SizedBox(
                height: 10,
              ),

              Text(
                '$temperature°C',

                style:
                    const TextStyle(
                  fontSize: 22,

                  fontWeight:
                      FontWeight.bold,
                ),
              ),

              const SizedBox(
                height: 6,
              ),

              Text(
                weather,

                maxLines: 1,

                overflow:
                    TextOverflow.ellipsis,

                textAlign:
                    TextAlign.center,

                style:
                    const TextStyle(
                  fontSize: 12,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // =====================================================
  // INFO CUACA
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

          size: 26,
        ),

        const SizedBox(
          height: 6,
        ),

        Text(
          label,

          style:
              const TextStyle(
            fontSize: 12,
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
    final text =
        weather.toLowerCase();

    if (text.contains('hujan')) {
      return Icon(
        Icons.umbrella,

        size: size,
      );
    }

    if (text.contains('petir') ||
        text.contains('badai')) {
      return Icon(
        Icons.thunderstorm,

        size: size,
      );
    }

    if (text.contains('cerah')) {
      return Icon(
        Icons.wb_sunny,

        size: size,
      );
    }

    if (text.contains('berawan')) {
      return Icon(
        Icons.cloud,

        size: size,
      );
    }

    return Icon(
      Icons.cloud_queue,

      size: size,
    );
  }

  // =====================================================
  // ERROR
  // =====================================================

  Widget _buildError() {
    return Container(
      padding:
          const EdgeInsets.all(
        12,
      ),

      decoration:
          BoxDecoration(
        color:
            Colors.red.shade50,

        borderRadius:
            BorderRadius.circular(
          8,
        ),
      ),

      child: Text(
        errorMessage,

        style:
            TextStyle(
          color:
              Colors.red.shade700,
        ),
      ),
    );
  }
}