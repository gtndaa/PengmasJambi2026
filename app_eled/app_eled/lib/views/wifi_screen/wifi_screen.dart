import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../services/device_config_service.dart';

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
  // SERVICE
  // =====================================================

  final DeviceConfigService
      deviceConfigService =
      DeviceConfigService();

  // =====================================================
  // CONTROLLERS
  // =====================================================

  final TextEditingController
      wifiSSIDController =
      TextEditingController();

  final TextEditingController
      wifiPasswordController =
      TextEditingController();

  final TextEditingController
      serverURLController =
      TextEditingController();

  final TextEditingController
      apiKeyController =
      TextEditingController();

  final TextEditingController
      uploadIntervalController =
      TextEditingController(
    text: '60000',
  );

  final TextEditingController
      listenWindowController =
      TextEditingController(
    text: '10000',
  );

  final TextEditingController
      configVersionController =
      TextEditingController(
    text: '1',
  );

  // =====================================================
  // STATUS
  // =====================================================

  bool useDeepSleep = false;

  bool isSending = false;

  bool isLoading = true;

  bool obscureWifiPassword = true;

  bool obscureApiKey = true;

  // =====================================================
  // TIMEZONE
  // =====================================================

  String selectedTimezone =
      'UTC+7';

  final Map<String, int>
      timezoneOffsets = {
    'UTC+7': 25200,
    'UTC+8': 28800,
    'UTC+9': 32400,
  };

  // =====================================================
  // INIT
  // =====================================================

  @override
  void initState() {
    super.initState();

    loadSavedConfiguration();
  }

  // =====================================================
  // LOAD DATA TERAKHIR
  // =====================================================

  Future<void>
      loadSavedConfiguration() async {
    final prefs =
        await SharedPreferences
            .getInstance();

    wifiSSIDController.text =
        prefs.getString(
              'wifiSSID',
            ) ??
            '';

    wifiPasswordController.text =
        prefs.getString(
              'wifiPassword',
            ) ??
            '';

    serverURLController.text =
        prefs.getString(
              'serverURL',
            ) ??
            '';

    apiKeyController.text =
        prefs.getString(
              'apiKey',
            ) ??
            '';

    uploadIntervalController
        .text = prefs.getString(
          'uploadInterval',
        ) ??
        '60000';

    listenWindowController
        .text = prefs.getString(
          'listenWindow',
        ) ??
        '10000';

    configVersionController
        .text = prefs.getString(
          'configVersion',
        ) ??
        '1';

    selectedTimezone =
        prefs.getString(
              'timezone',
            ) ??
            'UTC+7';

    useDeepSleep =
        prefs.getBool(
              'useDeepSleep',
            ) ??
            false;

    if (!mounted) return;

    setState(() {
      isLoading = false;
    });
  }

  // =====================================================
  // SIMPAN DATA LOKAL
  // =====================================================

  Future<void>
      saveConfigurationLocally() async {
    final prefs =
        await SharedPreferences
            .getInstance();

    await prefs.setString(
      'wifiSSID',
      wifiSSIDController.text,
    );

    await prefs.setString(
      'wifiPassword',
      wifiPasswordController.text,
    );

    await prefs.setString(
      'serverURL',
      serverURLController.text,
    );

    await prefs.setString(
      'apiKey',
      apiKeyController.text,
    );

    await prefs.setString(
      'uploadInterval',
      uploadIntervalController.text,
    );

    await prefs.setString(
      'listenWindow',
      listenWindowController.text,
    );

    await prefs.setString(
      'timezone',
      selectedTimezone,
    );

    await prefs.setString(
      'configVersion',
      configVersionController.text,
    );

    await prefs.setBool(
      'useDeepSleep',
      useDeepSleep,
    );
  }

  // =====================================================
  // BUAT DEVICE CONFIG
  // =====================================================

  Map<String, dynamic>
      buildDeviceConfig() {
    return {
      'wifiSSID':
          wifiSSIDController.text.trim(),

      'wifiPassword':
          wifiPasswordController.text,

      'serverURL':
          serverURLController.text.trim(),

      'apiKey':
          apiKeyController.text.trim(),

      'uploadInterval':
          int.parse(
        uploadIntervalController.text,
      ),

      'listenWindow':
          int.parse(
        listenWindowController.text,
      ),

      'timezoneOffset':
          timezoneOffsets[
              selectedTimezone],

      'configVersion':
          int.parse(
        configVersionController.text,
      ),

      'useDeepSleep':
          useDeepSleep,
    };
  }

  // =====================================================
  // VALIDASI
  // =====================================================

  bool validateInput() {
    if (wifiSSIDController.text
        .trim()
        .isEmpty) {
      showError(
        'WiFi SSID belum diisi.',
      );

      return false;
    }

    if (wifiPasswordController
        .text
        .isEmpty) {
      showError(
        'Password WiFi belum diisi.',
      );

      return false;
    }

    if (serverURLController.text
        .trim()
        .isEmpty) {
      showError(
        'Server URL belum diisi.',
      );

      return false;
    }

    if (apiKeyController.text
        .trim()
        .isEmpty) {
      showError(
        'API Key belum diisi.',
      );

      return false;
    }

    if (int.tryParse(
          uploadIntervalController
              .text,
        ) ==
        null) {
      showError(
        'Upload Interval harus berupa angka.',
      );

      return false;
    }

    if (int.tryParse(
          listenWindowController
              .text,
        ) ==
        null) {
      showError(
        'Listen Window harus berupa angka.',
      );

      return false;
    }

    if (int.tryParse(
          configVersionController
              .text,
        ) ==
        null) {
      showError(
        'Config Version harus berupa angka.',
      );

      return false;
    }

    return true;
  }

  // =====================================================
  // SIMPAN DAN KIRIM
  // =====================================================

  Future<void>
      saveAndSendConfiguration() async {
    if (!validateInput()) {
      return;
    }

    try {
      setState(() {
        isSending = true;
      });

      final config =
          buildDeviceConfig();

      print('================================');
      print('DEVICE CONFIGURATION');
      print(jsonEncode(config));
      print('================================');

      // Simpan lokal
      await saveConfigurationLocally();

      // Kirim ke cloud
      await deviceConfigService
          .sendDeviceConfig(
        serverURL:
            serverURLController.text
                .trim(),

        apiKey:
            apiKeyController.text
                .trim(),

        config: config,
      );

      if (!mounted) return;

      setState(() {
        isSending = false;
      });

      showSuccess(
        'Konfigurasi berhasil dikirim ke cloud.',
      );
    } catch (e) {
      if (!mounted) return;

      setState(() {
        isSending = false;
      });

      showError(
        'Gagal mengirim konfigurasi:\n$e',
      );
    }
  }

  // =====================================================
  // SNACKBAR SUCCESS
  // =====================================================

  void showSuccess(
    String message,
  ) {
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(
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
  // SNACKBAR ERROR
  // =====================================================

  void showError(
    String message,
  ) {
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(
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
  // DISPOSE
  // =====================================================

  @override
  void dispose() {
    wifiSSIDController.dispose();

    wifiPasswordController.dispose();

    serverURLController.dispose();

    apiKeyController.dispose();

    uploadIntervalController.dispose();

    listenWindowController.dispose();

    configVersionController.dispose();

    super.dispose();
  }

  // =====================================================
  // BUILD
  // =====================================================

  @override
  Widget build(
    BuildContext context,
  ) {
    if (isLoading) {
      return const Scaffold(
        body: Center(
          child:
              CircularProgressIndicator(),
        ),
      );
    }

    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'WiFi Configuration',
        ),
      ),

      body:
          SingleChildScrollView(
        padding:
            const EdgeInsets.all(
          20,
        ),

        child:
            Column(
          crossAxisAlignment:
              CrossAxisAlignment
                  .stretch,

          children: [
            _buildHeader(),

            const SizedBox(
              height: 24,
            ),

            _buildSectionTitle(
              'WiFi Settings',
              Icons.wifi,
            ),

            const SizedBox(
              height: 12,
            ),

            _buildTextField(
              controller:
                  wifiSSIDController,

              label:
                  'WiFi SSID',

              hint:
                  'Masukkan nama WiFi',

              icon:
                  Icons.wifi,
            ),

            const SizedBox(
              height: 16,
            ),

            _buildPasswordField(
              controller:
                  wifiPasswordController,

              label:
                  'WiFi Password',

              obscure:
                  obscureWifiPassword,

              onToggle: () {
                setState(() {
                  obscureWifiPassword =
                      !obscureWifiPassword;
                });
              },
            ),

            const SizedBox(
              height: 28,
            ),

            _buildSectionTitle(
              'Cloud Server',
              Icons.cloud,
            ),

            const SizedBox(
              height: 12,
            ),

            _buildTextField(
              controller:
                  serverURLController,

              label:
                  'Server URL',

              hint:
                  'https://example.com/api',

              icon:
                  Icons.link,

              keyboardType:
                  TextInputType.url,
            ),

            const SizedBox(
              height: 16,
            ),

            _buildPasswordField(
              controller:
                  apiKeyController,

              label:
                  'API Key',

              obscure:
                  obscureApiKey,

              onToggle: () {
                setState(() {
                  obscureApiKey =
                      !obscureApiKey;
                });
              },
            ),

            const SizedBox(
              height: 28,
            ),

            _buildSectionTitle(
              'Device Settings',
              Icons.settings,
            ),

            const SizedBox(
              height: 12,
            ),

            _buildNumberField(
              controller:
                  uploadIntervalController,

              label:
                  'Upload Interval',

              hint:
                  'Dalam milidetik',

              icon:
                  Icons.upload,
            ),

            const SizedBox(
              height: 16,
            ),

            _buildNumberField(
              controller:
                  listenWindowController,

              label:
                  'Listen Window',

              hint:
                  'Dalam milidetik',

              icon:
                  Icons.hearing,
            ),

            const SizedBox(
              height: 16,
            ),

            _buildTimezoneDropdown(),

            const SizedBox(
              height: 16,
            ),

            _buildNumberField(
              controller:
                  configVersionController,

              label:
                  'Config Version',

              hint:
                  'Contoh: 1',

              icon:
                  Icons.numbers,
            ),

            const SizedBox(
              height: 16,
            ),

            Card(
              child:
                  SwitchListTile(
                title:
                    const Text(
                  'Deep Sleep',
                ),

                subtitle:
                    const Text(
                  'Aktifkan mode hemat energi',
                ),

                secondary:
                    const Icon(
                  Icons.battery_saver,
                ),

                value:
                    useDeepSleep,

                onChanged:
                    (value) {
                  setState(() {
                    useDeepSleep =
                        value;
                  });
                },
              ),
            ),

            const SizedBox(
              height: 24,
            ),

            _buildConfigurationPreview(),

            const SizedBox(
              height: 24,
            ),

            SizedBox(
              height: 56,

              child:
                  ElevatedButton.icon(
                onPressed:
                    isSending
                        ? null
                        : saveAndSendConfiguration,

                icon:
                    isSending
                        ? const SizedBox(
                            width: 20,
                            height: 20,
                            child:
                                CircularProgressIndicator(
                              strokeWidth:
                                  2,
                            ),
                          )
                        : const Icon(
                            Icons.cloud_upload,
                          ),

                label:
                    Text(
                  isSending
                      ? 'Mengirim...'
                      : 'Simpan & Kirim Konfigurasi',
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // HEADER
  // =====================================================

  Widget _buildHeader() {
    return Card(
      elevation:
          2,

      child:
          Padding(
        padding:
            const EdgeInsets.all(
          20,
        ),

        child:
            Column(
          children: [
            const Icon(
              Icons.settings_remote,
              size: 56,
            ),

            const SizedBox(
              height: 12,
            ),

            const Text(
              'Device Configuration',

              style:
                  TextStyle(
                fontSize:
                    22,

                fontWeight:
                    FontWeight.bold,
              ),
            ),

            const SizedBox(
              height: 8,
            ),

            Text(
              'Atur konfigurasi koneksi dan operasi perangkat.',

              textAlign:
                  TextAlign.center,

              style:
                  TextStyle(
                color:
                    Colors.grey.shade600,
              ),
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // SECTION TITLE
  // =====================================================

  Widget _buildSectionTitle(
    String title,
    IconData icon,
  ) {
    return Row(
      children: [
        Icon(
          icon,
        ),

        const SizedBox(
          width: 8,
        ),

        Text(
          title,

          style:
              const TextStyle(
            fontSize:
                20,

            fontWeight:
                FontWeight.bold,
          ),
        ),
      ],
    );
  }

  // =====================================================
  // TEXT FIELD
  // =====================================================

  Widget _buildTextField({
    required TextEditingController
        controller,

    required String label,

    required String hint,

    required IconData icon,

    TextInputType keyboardType =
        TextInputType.text,
  }) {
    return TextField(
      controller:
          controller,

      keyboardType:
          keyboardType,

      decoration:
          InputDecoration(
        labelText:
            label,

        hintText:
            hint,

        prefixIcon:
            Icon(
          icon,
        ),

        border:
            const OutlineInputBorder(),
      ),
    );
  }

  // =====================================================
  // NUMBER FIELD
  // =====================================================

  Widget _buildNumberField({
    required TextEditingController
        controller,

    required String label,

    required String hint,

    required IconData icon,
  }) {
    return TextField(
      controller:
          controller,

      keyboardType:
          TextInputType.number,

      decoration:
          InputDecoration(
        labelText:
            label,

        hintText:
            hint,

        prefixIcon:
            Icon(
          icon,
        ),

        border:
            const OutlineInputBorder(),
      ),
    );
  }

  // =====================================================
  // PASSWORD FIELD
  // =====================================================

  Widget _buildPasswordField({
    required TextEditingController
        controller,

    required String label,

    required bool obscure,

    required VoidCallback onToggle,
  }) {
    return TextField(
      controller:
          controller,

      obscureText:
          obscure,

      decoration:
          InputDecoration(
        labelText:
            label,

        prefixIcon:
            const Icon(
          Icons.lock,
        ),

        suffixIcon:
            IconButton(
          icon:
              Icon(
            obscure
                ? Icons.visibility
                : Icons.visibility_off,
          ),

          onPressed:
              onToggle,
        ),

        border:
            const OutlineInputBorder(),
      ),
    );
  }

  // =====================================================
  // TIMEZONE DROPDOWN
  // =====================================================

  Widget _buildTimezoneDropdown() {
    return DropdownButtonFormField<
        String>(
      value:
          selectedTimezone,

      decoration:
          const InputDecoration(
        labelText:
            'Timezone',

        prefixIcon:
            Icon(
          Icons.access_time,
        ),

        border:
            OutlineInputBorder(),
      ),

      items:
          timezoneOffsets.keys
              .map(
        (timezone) {
          return DropdownMenuItem<
              String>(
            value:
                timezone,

            child:
                Text(
              timezone,
            ),
          );
        },
      ).toList(),

      onChanged:
          (value) {
        if (value == null) {
          return;
        }

        setState(() {
          selectedTimezone =
              value;
        });
      },
    );
  }

  // =====================================================
  // PREVIEW
  // =====================================================

  Widget _buildConfigurationPreview() {
    return Card(
      color:
          Colors.grey.shade100,

      child:
          Padding(
        padding:
            const EdgeInsets.all(
          16,
        ),

        child:
            Column(
          crossAxisAlignment:
              CrossAxisAlignment
                  .start,

          children: [
            const Text(
              'Configuration Preview',

              style:
                  TextStyle(
                fontSize:
                    18,

                fontWeight:
                    FontWeight.bold,
              ),
            ),

            const Divider(),

            _previewRow(
              'Upload Interval',

              '${uploadIntervalController.text} ms',
            ),

            _previewRow(
              'Listen Window',

              '${listenWindowController.text} ms',
            ),

            _previewRow(
              'Timezone',

              '$selectedTimezone '
                  '(${timezoneOffsets[selectedTimezone]} detik)',
            ),

            _previewRow(
              'Config Version',

              configVersionController.text,
            ),

            _previewRow(
              'Deep Sleep',

              useDeepSleep
                  ? 'Aktif'
                  : 'Nonaktif',
            ),
          ],
        ),
      ),
    );
  }

  // =====================================================
  // PREVIEW ROW
  // =====================================================

  Widget _previewRow(
    String label,
    String value,
  ) {
    return Padding(
      padding:
          const EdgeInsets.symmetric(
        vertical: 4,
      ),

      child:
          Row(
        mainAxisAlignment:
            MainAxisAlignment
                .spaceBetween,

        children: [
          Text(
            label,
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
      ),
    );
  }
}