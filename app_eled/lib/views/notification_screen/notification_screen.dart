import 'dart:async';

import 'package:flutter/material.dart';

import '../../models/weather_alert_model.dart';
import '../../services/bmkg_alert_service.dart';
import '../../services/location_storage_service.dart';

class NotificationScreen
    extends StatefulWidget {
  const NotificationScreen({
    super.key,
  });

  @override
  State<NotificationScreen>
      createState() =>
          _NotificationScreenState();
}

class _NotificationScreenState
    extends State<NotificationScreen>
    with WidgetsBindingObserver {
  final BmkgAlertService alertService =
      BmkgAlertService();

  List<WeatherAlert> alerts = [];

  bool isLoading = true;

  String errorMessage = '';

  String locationName = '';

  Timer? alertTimer;

  @override
  void initState() {
    super.initState();

    WidgetsBinding.instance
        .addObserver(this);

    _loadAlerts();

    // Cek warning BMKG setiap 5 menit
    alertTimer =
        Timer.periodic(
      const Duration(minutes: 5),
      (_) {
        _loadAlerts();
      },
    );
  }

  @override
  void dispose() {
    alertTimer?.cancel();

    WidgetsBinding.instance
        .removeObserver(this);

    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(
    AppLifecycleState state,
  ) {
    if (state ==
        AppLifecycleState.resumed) {
      _loadAlerts();
    }
  }

  Future<void> _loadAlerts() async {
    try {
      final location =
          await LocationStorageService
              .getLocation();

      if (location == null) {
        if (!mounted) return;

        setState(() {
          isLoading = false;

          alerts = [];

          locationName =
              'Lokasi belum dipilih';

          errorMessage = '';
        });

        return;
      }

      final province =
          location['province'] ?? '';

      final district =
          location['district'] ?? '';

      if (!mounted) return;

      setState(() {
        locationName =
            '$district, $province';
      });

      final result =
          await alertService
              .getActiveAlertsForLocation(
        province: province,
        district: district,
      );

      if (!mounted) return;

      setState(() {
        alerts = result;

        isLoading = false;

        errorMessage = '';
      });
    } catch (e) {
      if (!mounted) return;

      setState(() {
        isLoading = false;

        errorMessage =
            'Gagal mengambil peringatan BMKG.\n$e';
      });
    }
  }

  @override
  Widget build(
    BuildContext context,
  ) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Weather Alert',
        ),
      ),

      body: SafeArea(
        child: RefreshIndicator(
          onRefresh: _loadAlerts,

          child: ListView(
            physics:
                const AlwaysScrollableScrollPhysics(),

            padding:
                const EdgeInsets.all(20),

            children: [
              // ======================================
              // LOCATION
              // ======================================

              Row(
                children: [
                  const Icon(
                    Icons.location_on_outlined,
                    size: 20,
                  ),

                  const SizedBox(
                    width: 6,
                  ),

                  Expanded(
                    child: Text(
                      locationName,
                      style:
                          const TextStyle(
                        fontSize: 14,
                        fontWeight:
                            FontWeight.w500,
                      ),
                    ),
                  ),
                ],
              ),

              const SizedBox(
                height: 20,
              ),

              // ======================================
              // LOADING
              // ======================================

              if (isLoading)
                const Padding(
                  padding:
                      EdgeInsets.only(
                    top: 100,
                  ),
                  child: Center(
                    child:
                        CircularProgressIndicator(),
                  ),
                )

              // ======================================
              // ERROR
              // ======================================

              else if (errorMessage
                  .isNotEmpty)
                _buildErrorState()

              // ======================================
              // NO ALERT
              // ======================================

              else if (alerts.isEmpty)
                _buildSafeState()

              // ======================================
              // ALERT LIST
              // ======================================

              else ...[
                Text(
                  '${alerts.length} peringatan aktif',
                  style:
                      const TextStyle(
                    fontSize: 14,
                    color: Colors.grey,
                  ),
                ),

                const SizedBox(
                  height: 12,
                ),

                ...alerts.map(
                  _buildAlertCard,
                ),
              ],

              const SizedBox(
                height: 20,
              ),

              const Center(
                child: Text(
                  'Sumber data: BMKG',
                  style: TextStyle(
                    fontSize: 12,
                    color: Colors.grey,
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildSafeState() {
    return Card(
      child: Padding(
        padding:
            const EdgeInsets.all(28),
        child: Column(
          children: [
            const Icon(
              Icons.wb_sunny_outlined,
              size: 60,
              color: Colors.orange,
            ),

            const SizedBox(
              height: 16,
            ),

            const Text(
              'Kondisi Cuaca Aman',
              style: TextStyle(
                fontSize: 20,
                fontWeight:
                    FontWeight.bold,
              ),
            ),

            const SizedBox(
              height: 8,
            ),

            Text(
              'Tidak ada peringatan dini BMKG '
              'untuk $locationName saat ini.',
              textAlign:
                  TextAlign.center,
              style:
                  const TextStyle(
                color: Colors.grey,
              ),
            ),

            const SizedBox(
              height: 16,
            ),

            const Text(
              'Tarik ke bawah untuk memperbarui',
              style: TextStyle(
                fontSize: 12,
                color: Colors.grey,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildErrorState() {
    return Card(
      child: Padding(
        padding:
            const EdgeInsets.all(24),
        child: Column(
          children: [
            const Icon(
              Icons.cloud_off,
              size: 50,
              color: Colors.grey,
            ),

            const SizedBox(
              height: 12,
            ),

            Text(
              errorMessage,
              textAlign:
                  TextAlign.center,
            ),

            const SizedBox(
              height: 16,
            ),

            ElevatedButton(
              onPressed:
                  _loadAlerts,
              child:
                  const Text(
                'Coba Lagi',
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildAlertCard(
    WeatherAlert alert,
  ) {
    return Card(
      margin:
          const EdgeInsets.only(
        bottom: 16,
      ),
      child: Padding(
        padding:
            const EdgeInsets.all(18),
        child: Column(
          crossAxisAlignment:
              CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Container(
                  padding:
                      const EdgeInsets.all(
                    10,
                  ),
                  decoration:
                      BoxDecoration(
                    color:
                        Colors.red.shade50,
                    borderRadius:
                        BorderRadius.circular(
                      12,
                    ),
                  ),
                  child: const Icon(
                    Icons.warning_amber_rounded,
                    color: Colors.red,
                  ),
                ),

                const SizedBox(
                  width: 12,
                ),

                Expanded(
                  child: Text(
                    alert.title,
                    style:
                        const TextStyle(
                      fontSize: 17,
                      fontWeight:
                          FontWeight.bold,
                    ),
                  ),
                ),
              ],
            ),

            const SizedBox(
              height: 16,
            ),

            Text(
              alert.description,
              style:
                  const TextStyle(
                height: 1.5,
              ),
            ),

            const SizedBox(
              height: 14,
            ),

            Row(
              children: [
                const Icon(
                  Icons.verified_outlined,
                  size: 16,
                  color: Colors.grey,
                ),

                const SizedBox(
                  width: 5,
                ),

                Text(
                  'BMKG • ${alert.publishedAt}',
                  style:
                      const TextStyle(
                    fontSize: 12,
                    color: Colors.grey,
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}