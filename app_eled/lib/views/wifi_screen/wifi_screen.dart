import 'package:flutter/material.dart';

import '../../services/api_service.dart';
import '../../consts/admin_accounts.dart';
import '../../consts/colors.dart';
import '../../widgets/gradient_header.dart';

class WifiScreen extends StatefulWidget {
  const WifiScreen({super.key});

  @override
  State<WifiScreen> createState() => _WifiScreenState();
}

class _WifiScreenState extends State<WifiScreen> {
  // =====================================================
  // API SERVICE
  // =====================================================

  final ApiService apiService = ApiService();

  // =====================================================
  // WIFI CONTROLLERS
  // =====================================================

  final TextEditingController wifiSSIDController = TextEditingController(
    text: 'yaa gapunya data',
  );

  final TextEditingController wifiPasswordController = TextEditingController(
    text: 'hotspotgwserahgw',
  );

  // =====================================================
  // ADVANCED SETTINGS CONTROLLERS
  // =====================================================

  final TextEditingController uploadIntervalController = TextEditingController(
    text: '300000',
  );

  final TextEditingController listenWindowController = TextEditingController(
    text: '48000',
  );

  // Sleep interval & deep sleep tidak lagi diatur dari aplikasi:
  // selalu dikirim 0 (tidak tidur) dan true.
  static const int sleepInterval = 0;
  static const bool useDeepSleep = true;

  // =====================================================
  // STATE
  // =====================================================

  bool obscurePassword = true;

  bool isSending = false;

  bool isAdvancedExpanded = false;

  String connectedWifi = '';

  // =====================================================
  // INIT
  // =====================================================

  @override
  void initState() {
    super.initState();

    connectedWifi = wifiSSIDController.text.trim();
  }

  // =====================================================
  // DISPOSE
  // =====================================================

  @override
  void dispose() {
    wifiSSIDController.dispose();

    wifiPasswordController.dispose();

    uploadIntervalController.dispose();

    listenWindowController.dispose();

    super.dispose();
  }

  // =====================================================
  // SNACKBAR
  // =====================================================

  void showMessage({required String message, required bool isError}) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),

        backgroundColor: isError ? Colors.red : Colors.green,
      ),
    );
  }

  // =====================================================
  // PARSE INTEGER
  // =====================================================

  int? parseInteger(String value) {
    return int.tryParse(value.trim());
  }

  // =====================================================
  // KIRIM KONFIGURASI
  // =====================================================

  Future<void> sendConfiguration() async {
    if (!isAdminUser) {
      showMessage(
        message:
            'Akun ini tidak punya akses '
            'untuk mengubah konfigurasi',
        isError: true,
      );

      return;
    }

    final wifiSSID = wifiSSIDController.text.trim();

    final wifiPassword = wifiPasswordController.text;

    final uploadInterval = parseInteger(uploadIntervalController.text);

    final listenWindow = parseInteger(listenWindowController.text);

    // ===================================================
    // VALIDASI WIFI
    // ===================================================

    if (wifiSSID.isEmpty) {
      showMessage(message: 'WiFi SSID harus diisi', isError: true);

      return;
    }

    if (wifiPassword.isEmpty) {
      showMessage(message: 'Password WiFi harus diisi', isError: true);

      return;
    }

    // ===================================================
    // VALIDASI ADVANCED SETTINGS
    // ===================================================

    if (uploadInterval == null || uploadInterval < 0) {
      showMessage(
        message:
            'Upload interval '
            'harus berupa angka '
            'positif',
        isError: true,
      );

      return;
    }

    if (listenWindow == null || listenWindow < 0) {
      showMessage(
        message:
            'Listen window '
            'harus berupa angka '
            'positif',
        isError: true,
      );

      return;
    }

    // ===================================================
    // LOADING
    // ===================================================

    setState(() {
      isSending = true;
    });

    try {
      final result = await apiService.sendDeviceConfig(
        wifiSSID: wifiSSID,

        wifiPassword: wifiPassword,

        uploadInterval: uploadInterval,

        listenWindow: listenWindow,

        sleepInterval: sleepInterval,

        useDeepSleep: useDeepSleep,
      );

      if (!mounted) {
        return;
      }

      setState(() {
        isSending = false;

        connectedWifi = wifiSSID;
      });

      showMessage(
        message:
            result['message']?.toString() ??
            'Konfigurasi '
                'berhasil dikirim',
        isError: false,
      );
    } catch (error) {
      if (!mounted) {
        return;
      }

      setState(() {
        isSending = false;
      });

      showMessage(
        message:
            'Gagal mengirim '
            'konfigurasi:\n'
            '$error',
        isError: true,
      );
    }
  }

  // =====================================================
  // BUILD
  // =====================================================

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppColors.background,

      body: SafeArea(
        child: SingleChildScrollView(
          padding: const EdgeInsets.all(18),

          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,

            children: [
              const GradientHeader(
                icon: Icons.wifi,
                title: 'Konfigurasi Perangkat',
                subtitle: 'Atur koneksi WiFi dan pengaturan operasi ESP32.',
                colors: AppColors.blueGradient,
              ),
              const SizedBox(height: 18),

              // =========================================
              // HANYA AKUN ADMIN
              // =========================================
              if (!isAdminUser)
                _buildLockedNotice()
              else ...[
                // =========================================
                // WIFI STATUS
                // =========================================
                _buildWiFiStatus(),

                const SizedBox(height: 16),

                // =========================================
                // WIFI SETTINGS
                // =========================================
                Card(
                  child: Padding(
                    padding: const EdgeInsets.all(17),

                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.stretch,

                      children: [
                        const Row(
                          children: [
                            IconBadge(
                              icon: Icons.wifi,
                              color: AppColors.primary,
                              size: 36,
                            ),
                            SizedBox(width: 12),
                            Text(
                              'WiFi Settings',
                              style: TextStyle(
                                fontSize: 17,
                                fontWeight: FontWeight.w600,
                              ),
                            ),
                          ],
                        ),

                        const SizedBox(height: 18),

                        // =================================
                        // WIFI SSID
                        // =================================
                        TextField(
                          controller: wifiSSIDController,

                          decoration: const InputDecoration(
                            labelText: 'WiFi SSID',

                            prefixIcon: Icon(Icons.wifi),

                            border: OutlineInputBorder(),
                          ),
                        ),

                        const SizedBox(height: 15),

                        // =================================
                        // WIFI PASSWORD
                        // =================================
                        TextField(
                          controller: wifiPasswordController,

                          obscureText: obscurePassword,

                          decoration: InputDecoration(
                            labelText: 'WiFi Password',

                            prefixIcon: const Icon(Icons.lock),

                            border: const OutlineInputBorder(),

                            suffixIcon: IconButton(
                              onPressed: () {
                                setState(() {
                                  obscurePassword = !obscurePassword;
                                });
                              },

                              icon: Icon(
                                obscurePassword
                                    ? Icons.visibility
                                    : Icons.visibility_off,
                              ),
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),

                const SizedBox(height: 14),

                // =========================================
                // ADVANCED SETTINGS
                // =========================================
                Card(
                  child: ExpansionTile(
                    shape: const Border(),
                    collapsedShape: const Border(),
                    initiallyExpanded: isAdvancedExpanded,

                    onExpansionChanged: (value) {
                      setState(() {
                        isAdvancedExpanded = value;
                      });
                    },

                    leading: const IconBadge(
                      icon: Icons.tune,
                      color: Color(0xFF8E7CFF),
                      size: 36,
                    ),

                    title: const Text(
                      'Advanced Settings',
                      style: TextStyle(fontWeight: FontWeight.bold),
                    ),

                    subtitle: const Text(
                      'Upload interval '
                      'dan listen window',
                    ),

                    children: [
                      Padding(
                        padding: const EdgeInsets.fromLTRB(17, 5, 17, 18),

                        child: Column(
                          children: [
                            // =============================
                            // UPLOAD INTERVAL
                            // =============================
                            TextField(
                              controller: uploadIntervalController,

                              keyboardType: TextInputType.number,

                              decoration: const InputDecoration(
                                labelText: 'Upload Interval',

                                helperText: 'Satuan: ms',

                                prefixIcon: Icon(Icons.cloud_upload),

                                border: OutlineInputBorder(),
                              ),
                            ),

                            const SizedBox(height: 15),

                            // =============================
                            // LISTEN WINDOW
                            // =============================
                            TextField(
                              controller: listenWindowController,

                              keyboardType: TextInputType.number,

                              decoration: const InputDecoration(
                                labelText: 'Listen Window',

                                helperText: 'Satuan: ms',

                                prefixIcon: Icon(Icons.sensors),

                                border: OutlineInputBorder(),
                              ),
                            ),
                          ],
                        ),
                      ),
                    ],
                  ),
                ),

                const SizedBox(height: 22),

                // =========================================
                // SEND BUTTON
                // =========================================
                SizedBox(
                  height: 54,

                  child: ElevatedButton.icon(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: AppColors.primary,
                      foregroundColor: Colors.white,
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(16),
                      ),
                    ),
                    onPressed: isSending ? null : sendConfiguration,

                    icon: isSending
                        ? const SizedBox(
                            width: 22,

                            height: 22,

                            child: CircularProgressIndicator(
                              strokeWidth: 2,
                              color: Colors.white,
                            ),
                          )
                        : const Icon(Icons.cloud_upload),

                    label: Text(
                      isSending
                          ? 'MENGIRIM...'
                          : 'SIMPAN & KIRIM '
                                'KONFIGURASI',
                    ),
                  ),
                ),

                const SizedBox(height: 20),
              ],
            ],
          ),
        ),
      ),
    );
  }

  // =====================================================
  // AKSES DIBATASI
  // =====================================================

  Widget _buildLockedNotice() {
    return const Card(
      child: Padding(
        padding: EdgeInsets.all(20),
        child: Column(
          children: [
            Icon(Icons.lock_outline, size: 48, color: AppColors.primary),
            SizedBox(height: 12),
            Text(
              'Akses dibatasi',
              style: TextStyle(fontSize: 17, fontWeight: FontWeight.w600),
            ),
            SizedBox(height: 6),
            Text(
              'WiFi Settings dan Advanced Settings hanya bisa '
              'dibuka oleh akun pengelola perangkat. '
              'Hubungi admin jika kamu perlu mengubah konfigurasi.',
              textAlign: TextAlign.center,
              style: TextStyle(color: Colors.grey),
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // WIFI STATUS
  // =====================================================

  Widget _buildWiFiStatus() {
    final hasWiFi = connectedWifi.isNotEmpty;

    return Container(
      padding: const EdgeInsets.all(16),

      decoration: BoxDecoration(
        color: hasWiFi
            ? Colors.green.withValues(alpha: 0.10)
            : Colors.grey.withValues(alpha: 0.10),

        borderRadius: BorderRadius.circular(14),

        border: Border.all(color: hasWiFi ? Colors.green : Colors.grey),
      ),

      child: Row(
        children: [
          CircleAvatar(
            backgroundColor: hasWiFi ? Colors.green : Colors.grey,

            child: Icon(
              hasWiFi ? Icons.wifi : Icons.wifi_off,

              color: Colors.white,
            ),
          ),

          const SizedBox(width: 13),

          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,

              children: [
                Text(
                  hasWiFi ? 'Konfigurasi WiFi' : 'Belum ada WiFi',
                  style: const TextStyle(
                    fontWeight: FontWeight.bold,

                    fontSize: 16,
                  ),
                ),

                const SizedBox(height: 3),

                Text(
                  hasWiFi
                      ? connectedWifi
                      : 'Masukkan SSID '
                            'WiFi',
                ),
              ],
            ),
          ),

          Icon(
            hasWiFi ? Icons.check_circle : Icons.info_outline,

            color: hasWiFi ? Colors.green : Colors.grey,
          ),
        ],
      ),
    );
  }
}
