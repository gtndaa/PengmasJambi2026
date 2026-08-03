import 'package:flutter/material.dart';

import '../../services/api_service.dart';

class WifiScreen extends StatefulWidget {
  const WifiScreen({
    super.key,
  });

  @override
  State<WifiScreen> createState() =>
      _WifiScreenState();
}

class _WifiScreenState
    extends State<WifiScreen> {
  // =====================================================
  // API SERVICE
  // =====================================================

  final ApiService apiService =
      ApiService();

  // =====================================================
  // WIFI CONTROLLERS
  // =====================================================

  final TextEditingController
      wifiSsidController =
      TextEditingController(
    text: '',
  );

  final TextEditingController
      wifiPasswordController =
      TextEditingController(
    text: '',
  );

  // =====================================================
  // ADVANCED SETTINGS CONTROLLERS
  // =====================================================

  final TextEditingController
      uploadIntervalController =
      TextEditingController(
    text: '300000',
  );

  final TextEditingController
      listenWindowController =
      TextEditingController(
    text: '10000',
  );

  final TextEditingController
      timezoneOffsetController =
      TextEditingController(
    text: '25200',
  );

  // =====================================================
  // STATE
  // =====================================================

  bool useDeepSleep = true;

  bool obscurePassword = true;

  bool isSending = false;

  bool isAdvancedExpanded = false;

  // =====================================================
  // DISPOSE
  // =====================================================

  @override
  void dispose() {
    wifiSsidController.dispose();

    wifiPasswordController.dispose();

    uploadIntervalController
        .dispose();

    listenWindowController
        .dispose();

    timezoneOffsetController
        .dispose();

    super.dispose();
  }

  // =====================================================
  // SHOW ERROR
  // =====================================================

  void showError(
    String message,
  ) {
    ScaffoldMessenger.of(context)
        .showSnackBar(
      SnackBar(
        content: Text(
          message,
        ),
        backgroundColor:
            Colors.red,
      ),
    );
  }

  // =====================================================
  // SHOW SUCCESS
  // =====================================================

  void showSuccess(
    String message,
  ) {
    ScaffoldMessenger.of(context)
        .showSnackBar(
      SnackBar(
        content: Text(
          message,
        ),
        backgroundColor:
            Colors.green,
      ),
    );
  }

  // =====================================================
  // SAVE AND SEND CONFIGURATION
  // =====================================================

  Future<void>
      saveAndSendConfiguration() async {
    // ===================================================
    // VALIDASI WIFI SSID
    // ===================================================

    if (wifiSsidController
        .text
        .trim()
        .isEmpty) {
      showError(
        'WiFi SSID harus diisi',
      );

      return;
    }

    // ===================================================
    // VALIDASI WIFI PASSWORD
    // ===================================================

    if (wifiPasswordController
        .text
        .isEmpty) {
      showError(
        'Password WiFi harus diisi',
      );

      return;
    }

    // ===================================================
    // VALIDASI ANGKA
    // ===================================================

    final int? uploadInterval =
        int.tryParse(
      uploadIntervalController
          .text
          .trim(),
    );

    final int? listenWindow =
        int.tryParse(
      listenWindowController
          .text
          .trim(),
    );

    final int? timezoneOffset =
        int.tryParse(
      timezoneOffsetController
          .text
          .trim(),
    );

    if (uploadInterval == null ||
        uploadInterval <= 0) {
      showError(
        'Upload interval harus '
        'berupa angka lebih dari 0',
      );

      return;
    }

    if (listenWindow == null ||
        listenWindow <= 0) {
      showError(
        'Listen window harus '
        'berupa angka lebih dari 0',
      );

      return;
    }

    if (timezoneOffset == null) {
      showError(
        'Timezone offset harus '
        'berupa angka',
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
      // =================================================
      // FORMAT JSON YANG DIKIRIM KE CLOUD
      // =================================================

      final Map<String, dynamic>
          config = {
        'wifiSSID':
            wifiSsidController
                .text
                .trim(),

        'wifiPassword':
            wifiPasswordController
                .text,

        'uploadInterval':
            uploadInterval,

        'listenWindow':
            listenWindow,

        'timezoneOffset':
            timezoneOffset,

        'useDeepSleep':
            useDeepSleep,
      };

      // =================================================
      // CETAK DATA KE DEBUG CONSOLE
      // =================================================

      debugPrint(
        '================================',
      );

      debugPrint(
        'DATA YANG AKAN DIKIRIM:',
      );

      debugPrint(
        config.toString(),
      );

      debugPrint(
        '================================',
      );

      // =================================================
      // KIRIM KE CLOUD
      // =================================================

      final result =
          await apiService
              .sendDeviceConfig(
        config: config,
      );

      if (!mounted) {
        return;
      }

      setState(() {
        isSending = false;
      });

      showSuccess(
        result['message'] ??
            'Konfigurasi berhasil '
                'dikirim ke cloud',
      );
    } catch (error) {
      if (!mounted) {
        return;
      }

      setState(() {
        isSending = false;
      });

      showError(
        'Gagal mengirim konfigurasi:\n'
        '$error',
      );
    }
  }

  // =====================================================
  // BUILD
  // =====================================================

  @override
  Widget build(
    BuildContext context,
  ) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'WiFi Settings',
        ),
      ),

      body: SafeArea(
        child:
            SingleChildScrollView(
          padding:
              const EdgeInsets.all(
            20,
          ),

          child: Column(
            crossAxisAlignment:
                CrossAxisAlignment
                    .stretch,

            children: [
              // =========================================
              // HEADER
              // =========================================

              const Icon(
                Icons.wifi,
                size: 70,
                color: Colors.blue,
              ),

              const SizedBox(
                height: 16,
              ),

              const Text(
                'Pengaturan WiFi',
                textAlign:
                    TextAlign.center,
                style:
                    TextStyle(
                  fontSize: 24,
                  fontWeight:
                      FontWeight.bold,
                ),
              ),

              const SizedBox(
                height: 8,
              ),

              const Text(
                'Masukkan informasi WiFi '
                'dan konfigurasi perangkat.',
                textAlign:
                    TextAlign.center,
              ),

              const SizedBox(
                height: 28,
              ),

              // =========================================
              // WIFI CARD
              // =========================================

              Card(
                child: Padding(
                  padding:
                      const EdgeInsets.all(
                    18,
                  ),

                  child: Column(
                    crossAxisAlignment:
                        CrossAxisAlignment
                            .stretch,

                    children: [
                      const Row(
                        children: [
                          Icon(
                            Icons
                                .wifi_outlined,
                          ),

                          SizedBox(
                            width: 10,
                          ),

                          Text(
                            'WiFi Settings',
                            style:
                                TextStyle(
                              fontSize:
                                  18,
                              fontWeight:
                                  FontWeight
                                      .bold,
                            ),
                          ),
                        ],
                      ),

                      const SizedBox(
                        height: 20,
                      ),

                      // =================================
                      // WIFI SSID
                      // =================================

                      TextField(
                        controller:
                            wifiSsidController,

                        decoration:
                            const InputDecoration(
                          labelText:
                              'WiFi SSID',

                          hintText:
                              'Masukkan nama WiFi',

                          prefixIcon:
                              Icon(
                            Icons
                                .wifi,
                          ),

                          border:
                              OutlineInputBorder(),
                        ),
                      ),

                      const SizedBox(
                        height: 18,
                      ),

                      // =================================
                      // WIFI PASSWORD
                      // =================================

                      TextField(
                        controller:
                            wifiPasswordController,

                        obscureText:
                            obscurePassword,

                        decoration:
                            InputDecoration(
                          labelText:
                              'WiFi Password',

                          hintText:
                              'Masukkan password WiFi',

                          prefixIcon:
                              const Icon(
                            Icons.lock,
                          ),

                          border:
                              const OutlineInputBorder(),

                          suffixIcon:
                              IconButton(
                            onPressed:
                                () {
                              setState(
                                () {
                                  obscurePassword =
                                      !obscurePassword;
                                },
                              );
                            },

                            icon:
                                Icon(
                              obscurePassword
                                  ? Icons
                                      .visibility
                                  : Icons
                                      .visibility_off,
                            ),
                          ),
                        ),
                      ),
                    ],
                  ),
                ),
              ),

              const SizedBox(
                height: 18,
              ),

              // =========================================
              // ADVANCED SETTINGS
              // =========================================

              Card(
                child:
                    ExpansionTile(
                  initiallyExpanded:
                      isAdvancedExpanded,

                  onExpansionChanged:
                      (value) {
                    setState(
                      () {
                        isAdvancedExpanded =
                            value;
                      },
                    );
                  },

                  leading:
                      const Icon(
                    Icons.settings,
                  ),

                  title:
                      const Text(
                    'Advanced Settings',
                    style:
                        TextStyle(
                      fontWeight:
                          FontWeight.bold,
                    ),
                  ),

                  subtitle:
                      const Text(
                    'Pengaturan interval, '
                    'timezone, dan deep sleep',
                  ),

                  childrenPadding:
                      const EdgeInsets.fromLTRB(
                    18,
                    0,
                    18,
                    18,
                  ),

                  children: [
                    // ===============================
                    // UPLOAD INTERVAL
                    // ===============================

                    TextField(
                      controller:
                          uploadIntervalController,

                      keyboardType:
                          TextInputType
                              .number,

                      decoration:
                          const InputDecoration(
                        labelText:
                            'Upload Interval',

                        hintText:
                            'Contoh: 300000',

                        helperText:
                            'Satuan: milidetik '
                            '(ms)',

                        prefixIcon:
                            Icon(
                          Icons
                              .cloud_upload_outlined,
                        ),

                        border:
                            OutlineInputBorder(),
                      ),
                    ),

                    const SizedBox(
                      height: 18,
                    ),

                    // ===============================
                    // LISTEN WINDOW
                    // ===============================

                    TextField(
                      controller:
                          listenWindowController,

                      keyboardType:
                          TextInputType
                              .number,

                      decoration:
                          const InputDecoration(
                        labelText:
                            'Listen Window',

                        hintText:
                            'Contoh: 10000',

                        helperText:
                            'Satuan: milidetik '
                            '(ms)',

                        prefixIcon:
                            Icon(
                          Icons
                              .sensors,
                        ),

                        border:
                            OutlineInputBorder(),
                      ),
                    ),

                    const SizedBox(
                      height: 18,
                    ),

                    // ===============================
                    // TIMEZONE OFFSET
                    // ===============================

                    TextField(
                      controller:
                          timezoneOffsetController,

                      keyboardType:
                          const TextInputType
                              .numberWithOptions(
                        signed:
                            true,
                      ),

                      decoration:
                          const InputDecoration(
                        labelText:
                            'Timezone Offset',

                        hintText:
                            'Contoh: 25200',

                        helperText:
                            'Satuan: detik '
                            '(WIB = 25200)',

                        prefixIcon:
                            Icon(
                          Icons
                              .schedule,
                        ),

                        border:
                            OutlineInputBorder(),
                      ),
                    ),

                    const SizedBox(
                      height: 10,
                    ),

                    // ===============================
                    // DEEP SLEEP
                    // ===============================

                    SwitchListTile(
                      contentPadding:
                          EdgeInsets.zero,

                      title:
                          const Text(
                        'Use Deep Sleep',
                      ),

                      subtitle:
                          Text(
                        useDeepSleep
                            ? 'Deep sleep aktif'
                            : 'Deep sleep tidak aktif',
                      ),

                      secondary:
                          const Icon(
                        Icons
                            .bedtime_outlined,
                      ),

                      value:
                          useDeepSleep,

                      onChanged:
                          (value) {
                        setState(
                          () {
                            useDeepSleep =
                                value;
                          },
                        );
                      },
                    ),
                  ],
                ),
              ),

              const SizedBox(
                height: 26,
              ),

              // =========================================
              // SEND BUTTON
              // =========================================

              SizedBox(
                height: 52,

                child:
                    ElevatedButton.icon(
                  onPressed:
                      isSending
                          ? null
                          : saveAndSendConfiguration,

                  icon:
                      isSending
                          ? const SizedBox(
                              width: 22,
                              height: 22,
                              child:
                                  CircularProgressIndicator(
                                strokeWidth:
                                    2,
                              ),
                            )
                          : const Icon(
                              Icons
                                  .cloud_upload,
                            ),

                  label:
                      Text(
                    isSending
                        ? 'MENGIRIM...'
                        : 'SIMPAN DAN KIRIM',
                  ),
                ),
              ),

              const SizedBox(
                height: 20,
              ),

              // =========================================
              // INFORMATION
              // =========================================

              Container(
                padding:
                    const EdgeInsets.all(
                  14,
                ),

                decoration:
                    BoxDecoration(
                  color:
                      Colors.blue
                          .withValues(
                    alpha: 0.08,
                  ),

                  borderRadius:
                      BorderRadius.circular(
                    12,
                  ),
                ),

                child:
                    const Row(
                  crossAxisAlignment:
                      CrossAxisAlignment
                          .start,

                  children: [
                    Icon(
                      Icons.info_outline,
                    ),

                    SizedBox(
                      width: 10,
                    ),

                    Expanded(
                      child:
                          Text(
                        'Konfigurasi akan '
                        'dikirim ke cloud '
                        'melalui API.',
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}