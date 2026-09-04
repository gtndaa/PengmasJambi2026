import 'dart:async';

import 'package:flutter/material.dart';

import '../../services/api_service.dart';

class BmkgScreen
    extends StatefulWidget {
  const BmkgScreen({
    super.key,
  });

  @override
  State<BmkgScreen>
      createState() =>
          _BmkgScreenState();
}

class _BmkgScreenState
    extends State<BmkgScreen> {
  // =====================================================
  // API
  // =====================================================

  final ApiService apiService =
      ApiService();

  // =====================================================
  // TIMER
  // =====================================================

  Timer? sensorDataTimer;

  Timer? configCheckTimer;

  // =====================================================
  // DEFAULT CONFIGURATION
  // =====================================================

  int uploadIntervalMs =
      300000;

  int configVersion =
      -1;

  // =====================================================
  // STATUS
  // =====================================================

  bool isLoading =
      true;

  String? errorMessage;

  Map<String, dynamic>?
      sensorData;

  DateTime?
      lastUpdated;

  // =====================================================
  // INIT STATE
  // =====================================================

  @override
  void initState() {
    super.initState();

    initializeScreen();
  }

  // =====================================================
  // INITIALIZE
  // =====================================================

  Future<void>
      initializeScreen() async {
    // Ambil data sensor
    // langsung saat halaman dibuka.

    await loadSensorData();

    // Ambil konfigurasi
    // dan buat timer.

    await loadConfiguration();

    // Periksa konfigurasi
    // setiap 30 detik.

    startConfigCheckTimer();
  }

  // =====================================================
  // LOAD CONFIGURATION
  // =====================================================

  Future<void>
      loadConfiguration() async {
    try {
      final config =
          await apiService
              .getDeviceConfig();

      final newInterval =
          int.tryParse(
        config[
                'uploadInterval']
            .toString(),
      );

      final newVersion =
          int.tryParse(
        config[
                'configVersion']
            .toString(),
      );

      if (newInterval ==
              null ||
          newInterval <=
              0) {
        throw Exception(
          'uploadInterval '
          'tidak valid',
        );
      }

      final intervalChanged =
          newInterval !=
              uploadIntervalMs;

      final versionChanged =
          newVersion !=
              null &&
              newVersion !=
                  configVersion;

      // Simpan konfigurasi
      // terbaru.

      uploadIntervalMs =
          newInterval;

      if (newVersion !=
          null) {
        configVersion =
            newVersion;
      }

      // Jika interval berubah
      // atau timer belum dibuat,
      // buat timer baru.

      if (intervalChanged ||
          versionChanged ||
          sensorDataTimer ==
              null) {
        startSensorDataTimer();
      }

      print(
        'UPLOAD INTERVAL: '
        '$uploadIntervalMs ms',
      );

      print(
        'CONFIG VERSION: '
        '$configVersion',
      );
    } catch (error) {
      print(
        'GAGAL MEMBACA CONFIG: '
        '$error',
      );

      // Jika config gagal,
      // gunakan interval default.

      if (sensorDataTimer ==
          null) {
        startSensorDataTimer();
      }
    }
  }

  // =====================================================
  // SENSOR TIMER
  // =====================================================

  void startSensorDataTimer() {
    // Hentikan timer lama.

    sensorDataTimer?.cancel();

    // Buat timer baru.

    sensorDataTimer =
        Timer.periodic(
      Duration(
        milliseconds:
            uploadIntervalMs,
      ),
      (timer) {
        loadSensorData();
      },
    );

    print(
      'SENSOR TIMER AKTIF: '
      '$uploadIntervalMs ms',
    );
  }

  // =====================================================
  // CONFIG CHECK TIMER
  // =====================================================

  void startConfigCheckTimer() {
    configCheckTimer?.cancel();

    configCheckTimer =
        Timer.periodic(
      const Duration(
        seconds: 30,
      ),
      (timer) {
        loadConfiguration();
      },
    );
  }

  // =====================================================
  // LOAD SENSOR DATA
  // =====================================================

  Future<void>
      loadSensorData() async {
    try {
      final data =
          await apiService
              .getSensorData();

      if (!mounted) {
        return;
      }

      setState(() {
        sensorData =
            data;

        lastUpdated =
            DateTime.now();

        isLoading =
            false;

        errorMessage =
            null;
      });
    } catch (error) {
      if (!mounted) {
        return;
      }

      setState(() {
        isLoading =
            false;

        errorMessage =
            error.toString();
      });
    }
  }

  // =====================================================
  // REFRESH
  // =====================================================

  Future<void>
      refreshData() async {
    await loadConfiguration();

    await loadSensorData();
  }

  // =====================================================
  // DISPOSE
  // =====================================================

  @override
  void dispose() {
    sensorDataTimer
        ?.cancel();

    configCheckTimer
        ?.cancel();

    super.dispose();
  }

  // =====================================================
  // BUILD
  // =====================================================

  @override
  Widget build(
    BuildContext context,
  ) {
    return Scaffold(
      appBar:
          AppBar(
        title:
            const Text(
          'Data Lahan',
        ),

        actions: [
          IconButton(
            onPressed:
                refreshData,

            icon:
                const Icon(
              Icons.refresh,
            ),

            tooltip:
                'Perbarui data',
          ),
        ],
      ),

      body:
          RefreshIndicator(
        onRefresh:
            refreshData,

        child:
            buildBody(),
      ),
    );
  }

  // =====================================================
  // BODY
  // =====================================================

  Widget buildBody() {
    if (isLoading &&
        sensorData ==
            null) {
      return const Center(
        child:
            CircularProgressIndicator(),
      );
    }

    if (errorMessage !=
            null &&
        sensorData ==
            null) {
      return ListView(
        padding:
            const EdgeInsets.all(
          24,
        ),

        children: [
          const SizedBox(
            height:
                80,
          ),

          Icon(
            Icons
                .cloud_off,
            size:
                70,
            color:
                Colors.red
                    .shade400,
          ),

          const SizedBox(
            height:
                18,
          ),

          const Text(
            'Gagal mengambil '
            'data lahan',
            textAlign:
                TextAlign.center,
            style:
                TextStyle(
              fontSize:
                  20,
              fontWeight:
                  FontWeight.bold,
            ),
          ),

          const SizedBox(
            height:
                10,
          ),

          Text(
            errorMessage!,
            textAlign:
                TextAlign.center,
          ),

          const SizedBox(
            height:
                20,
          ),

          ElevatedButton.icon(
            onPressed:
                refreshData,

            icon:
                const Icon(
              Icons.refresh,
            ),

            label:
                const Text(
              'COBA LAGI',
            ),
          ),
        ],
      );
    }

    final data =
        sensorData ??
            {};

    return ListView(
      padding:
          const EdgeInsets.all(
        16,
      ),

      children: [
        // ===============================================
        // STATUS
        // ===============================================

        Card(
          child:
              Padding(
            padding:
                const EdgeInsets.all(
              16,
            ),

            child:
                Row(
              children: [
                const CircleAvatar(
                  child:
                      Icon(
                    Icons
                        .sensors,
                  ),
                ),

                const SizedBox(
                  width:
                      14,
                ),

                Expanded(
                  child:
                      Column(
                    crossAxisAlignment:
                        CrossAxisAlignment
                            .start,

                    children: [
                      const Text(
                        'Monitoring '
                        'Lahan Aktif',
                        style:
                            TextStyle(
                          fontWeight:
                              FontWeight.bold,

                          fontSize:
                              17,
                        ),
                      ),

                      const SizedBox(
                        height:
                            4,
                      ),

                      Text(
                        'Update setiap '
                        '${formatInterval(
                          uploadIntervalMs,
                        )}',
                      ),
                    ],
                  ),
                ),

                const Icon(
                  Icons
                      .check_circle,
                  color:
                      Colors.green,
                ),
              ],
            ),
          ),
        ),

        const SizedBox(
          height:
              15,
        ),

        // ===============================================
        // DEVICE INFORMATION
        // ===============================================

        buildSectionTitle(
          'Informasi Device',
        ),

        const SizedBox(
          height:
              8,
        ),

        buildDataCard(
          icon:
              Icons
                  .memory,
          title:
              'Device ID',
          value:
              getValue(
            data,
            'id',
          ),
        ),

        buildDataCard(
          icon:
              Icons
                  .schedule,
          title:
              'Waktu Data',
          value:
              getValue(
            data,
            'datetime',
          ),
        ),

        // Battery Init
        // buildDataCard(
        //   icon:
        //       Icons
        //           .battery_full,
        //   title:
        //       'Status Baterai',
        //   value:
        //       getValue(
        //     data,
        //     'batt',
        //   ),
        // ),

        buildPowerStatusCard(
          status: getValue(
            data,
              'batt',
          ),
        ),

        // Buat testing on_cap 
        // buildPowerStatusCard(
        //   status: 'on_cap',
        // ),

        const SizedBox(
          height:
              15,
        ),

        // ===============================================
        // TEMPERATURE
        // ===============================================

        buildSectionTitle(
          'Kondisi Lingkungan',
        ),

        const SizedBox(
          height:
              8,
        ),

        buildDataCard(
          icon:
              Icons
                  .thermostat,
          title:
              'Suhu',
          value:
              '${getValue(
            data,
            'temp_out',
          )} °C',
        ),

        buildDataCard(
          icon:
              Icons
                  .water_drop,
          title:
              'Kelembapan',
          value:
              '${getValue(
            data,
            'hum_out',
          )} %',
        ),

        buildDataCard(
          icon:
              Icons
                  .light_mode,
          title:
              'Intensitas Cahaya',
          value:
              '${getValue(
            data,
            'light_lux',
          )} lux',
        ),

        const SizedBox(
          height:
              15,
        ),

        // ===============================================
        // WIND
        // ===============================================

        buildSectionTitle(
          'Kondisi Angin',
        ),

        const SizedBox(
          height:
              8,
        ),

        buildDataCard(
          icon:
              Icons
                  .air,
          title:
              'Kecepatan Angin',
          value:
              '${getValue(
            data,
            'wind_speed',
          )} m/s',
        ),

        buildDataCard(
          icon:
              Icons
                  .speed,
          title:
              'Wind Gust',
          value:
              '${getValue(
            data,
            'wind_gust',
          )} m/s',
        ),

        buildDataCard(
          icon:
              Icons
                  .explore,
          title:
              'Arah Angin',
          value:
              '${getValue(
            data,
            'wind_dir',
          )} '
              '(${getValue(
            data,
            'wind_deg',
          )}°)',
        ),

        const SizedBox(
          height:
              15,
        ),

        // ===============================================
        // RAIN
        // ===============================================

        buildSectionTitle(
          'Curah Hujan',
        ),

        const SizedBox(
          height:
              8,
        ),

        buildDataCard(
          icon:
              Icons
                  .umbrella,
          title:
              'Curah Hujan 1 Jam Terakhir',
          value:
              '${getValue(
            data,
            'rain_delta',
          )} mm',
        ),

        const SizedBox(
          height:
              20,
        ),

        // ===============================================
        // LAST UPDATE
        // ===============================================

        Center(
          child:
              Text(
            lastUpdated ==
                    null
                ? ''
                : 'Terakhir '
                    'diperbarui: '
                    '${formatDateTime(
                      lastUpdated!,
                    )}',

            style:
                TextStyle(
              color:
                  Colors
                      .grey
                      .shade600,

              fontSize:
                  12,
            ),
          ),
        ),

        const SizedBox(
          height:
              25,
        ),
      ],
    );
  }

  // =====================================================
  // SECTION TITLE
  // =====================================================

  Widget buildSectionTitle(
    String title,
  ) {
    return Text(
      title,
      style:
          const TextStyle(
        fontSize:
            18,
        fontWeight:
            FontWeight.bold,
      ),
    );
  }

  // =====================================================
  // DATA CARD
  // =====================================================

  Widget buildDataCard({
    required IconData icon,
    required String title,
    required String value,
  }) {
    return Card(
      margin:
          const EdgeInsets.only(
        bottom:
            9,
      ),

      child:
          ListTile(
        leading:
            CircleAvatar(
          child:
              Icon(
            icon,
          ),
        ),

        title:
            Text(
          title,
        ),

        trailing:
            Text(
          value,
          style:
              const TextStyle(
            fontWeight:
                FontWeight.bold,
          ),
        ),
      ),
    );
  }

// =====================================================
// POWER STATUS CARD
// =====================================================

Widget buildPowerStatusCard({
  required String status,
}) {
  final normalizedStatus =
      status.trim().toUpperCase();

  String title;
  String description;
  IconData icon;
  Color statusColor;

  switch (normalizedStatus) {
    // ===============================
    // NORMAL
    // ===============================
    case 'OK':
      title = 'Daya Normal';
      description =
          'Listrik terhubung dengan baik. Capbank sedang mengisi atau daya perangkat berada dalam kondisi aman.';
      icon = Icons.check_circle;
      statusColor = Colors.green;
      break;

    // ===============================
    // USING BACKUP POWER
    // ===============================
    case 'ON_CAP':
      title = 'Suplai Cadangan Aktif';
      description =
          'Listrik PLN tidak tersedia. Perangkat sedang menggunakan suplai daya cadangan.';
      icon = Icons.battery_saver;
      statusColor = Colors.orange;
      break;

    // ===============================
    // LOW POWER
    // ===============================
    case 'LOW':
      title = 'Daya Cadangan Menipis';
      description =
          'Daya cadangan mulai menipis. Periksa suplai listrik yang masuk ke alat dan pastikan sambungan listrik PLN berfungsi dengan baik.';
      icon = Icons.battery_2_bar;
      statusColor = Colors.orange;
      break;

    // ===============================
    // CRITICAL POWER
    // ===============================
    case 'CRIT':
      title = 'Daya Kritis';
      description =
          'Daya cadangan hampir habis. Perangkat berisiko mati dalam waktu dekat. Segera periksa dan pulihkan suplai listrik ke alat.';
      icon = Icons.battery_alert;
      statusColor = Colors.red;
      break;

    // ===============================
    // UNKNOWN STATUS
    // ===============================
    default:
      title = 'Status Daya Tidak Diketahui';
      description =
          'Status daya perangkat belum dapat dikenali. Periksa kondisi perangkat apabila diperlukan.';
      icon = Icons.help_outline;
      statusColor = Colors.grey;
  }

  return Card(
    margin: const EdgeInsets.only(
      bottom: 9,
    ),
    child: ListTile(
      leading: CircleAvatar(
        backgroundColor:
            statusColor.withOpacity(0.12),
        child: Icon(
          icon,
          color: statusColor,
        ),
      ),

      title: Text(
        title,
        style: const TextStyle(
          fontWeight: FontWeight.w600,
        ),
      ),

      subtitle: Padding(
        padding: const EdgeInsets.only(
          top: 4,
        ),
        child: Text(
          description,
        ),
      ),

      trailing: Icon(
        icon,
        color: statusColor,
      ),
    ),
  );
}

  // =====================================================
  // GET VALUE
  // =====================================================

  String getValue(
    Map<String, dynamic> data,
    String key,
  ) {
    final value =
        data[key];

    if (value ==
        null) {
      return '-';
    }

    return value
        .toString();
  }

  // =====================================================
  // FORMAT INTERVAL
  // =====================================================

  String formatInterval(
    int milliseconds,
  ) {
    final seconds =
        milliseconds ~/
            1000;

    if (seconds <
        60) {
      return '$seconds detik';
    }

    final minutes =
        seconds ~/
            60;

    if (minutes <
        60) {
      return '$minutes menit';
    }

    final hours =
        minutes ~/
            60;

    return '$hours jam';
  }

  // =====================================================
  // FORMAT DATE
  // =====================================================

  String formatDateTime(
    DateTime time,
  ) {
    final day =
        time.day
            .toString()
            .padLeft(
              2,
              '0',
            );

    final month =
        time.month
            .toString()
            .padLeft(
              2,
              '0',
            );

    final year =
        time.year;

    final hour =
        time.hour
            .toString()
            .padLeft(
              2,
              '0',
            );

    final minute =
        time.minute
            .toString()
            .padLeft(
              2,
              '0',
            );

    final second =
        time.second
            .toString()
            .padLeft(
              2,
              '0',
            );

    return '$day/$month/$year '
        '$hour:$minute:$second';
  }
}