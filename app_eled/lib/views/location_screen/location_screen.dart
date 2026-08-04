import 'package:flutter/material.dart';

import '../../services/wilayah_local_service.dart';

import '../../services/location_storage_service.dart';

class LocationScreen extends StatefulWidget {
  const LocationScreen({
    super.key,
  });

  @override
  State<LocationScreen> createState() =>
      _LocationScreenState();
}

class _LocationScreenState
    extends State<LocationScreen> {
  // =====================================================
  // SERVICE
  // =====================================================

  final WilayahLocalService wilayahService =
      WilayahLocalService();

  // =====================================================
  // DATA WILAYAH
  // =====================================================

  List<WilayahData> provinces = [];

  List<WilayahData> regencies = [];

  List<WilayahData> districts = [];

  List<WilayahData> villages = [];

  // =====================================================
  // PILIHAN
  // =====================================================

  WilayahData? selectedProvince;

  WilayahData? selectedRegency;

  WilayahData? selectedDistrict;

  WilayahData? selectedVillage;

  // =====================================================
  // STATUS
  // =====================================================

  bool isLoading = true;

  String errorMessage = '';

  // =====================================================
  // INIT
  // =====================================================

  @override
  void initState() {
    super.initState();

    _loadDatabase();
  }

  // =====================================================
  // LOAD DATABASE
  // =====================================================

  Future<void> _loadDatabase() async {
    try {
      await wilayahService.loadDatabase();

      final data =
          wilayahService.getProvinces();

      if (!mounted) return;

      setState(() {
        provinces = data;

        isLoading = false;
      });
    } catch (e) {
      if (!mounted) return;

      setState(() {
        isLoading = false;

        errorMessage =
            'Gagal memuat database wilayah:\n$e';
      });
    }
  }

  // =====================================================
  // PROVINSI
  // =====================================================

  void _onProvinceChanged(
    WilayahData? value,
  ) {
    if (value == null) return;

    setState(() {
      selectedProvince = value;

      selectedRegency = null;

      selectedDistrict = null;

      selectedVillage = null;

      regencies =
          wilayahService.getRegencies(
        value.code,
      );

      districts = [];

      villages = [];
    });
  }

  // =====================================================
  // KABUPATEN / KOTA
  // =====================================================

  void _onRegencyChanged(
    WilayahData? value,
  ) {
    if (value == null) return;

    setState(() {
      selectedRegency = value;

      selectedDistrict = null;

      selectedVillage = null;

      districts =
          wilayahService.getDistricts(
        value.code,
      );

      villages = [];
    });
  }

  // =====================================================
  // KECAMATAN
  // =====================================================

  void _onDistrictChanged(
    WilayahData? value,
  ) {
    if (value == null) return;

    setState(() {
      selectedDistrict = value;

      selectedVillage = null;

      villages =
          wilayahService.getVillages(
        value.code,
      );
    });
  }

  // =====================================================
  // DESA / KELURAHAN
  // =====================================================

  void _onVillageChanged(
    WilayahData? value,
  ) {
    if (value == null) return;

    setState(() {
      selectedVillage = value;
    });
  }

  // =====================================================
  // SIMPAN LOKASI
  // =====================================================

  Future<void> _saveLocation() async {
    if (selectedProvince == null ||
        selectedRegency == null ||
        selectedDistrict == null ||
        selectedVillage == null) {
      setState(() {
        errorMessage =
            'Silakan pilih lokasi secara lengkap.';
      });

      return;
    }

    await LocationStorageService
        .saveLocation(
      province:
          selectedProvince!.name,

      regency:
          selectedRegency!.name,

      district:
          selectedDistrict!.name,

      village:
          selectedVillage!.name,

      adm4:
          selectedVillage!.code,
    );

    if (!mounted) return;

    Navigator.pop(
      context,
      {
        'province':
            selectedProvince!.name,

        'regency':
            selectedRegency!.name,

        'district':
            selectedDistrict!.name,

        'village':
            selectedVillage!.name,

        'adm4':
            selectedVillage!.code,
      },
    );
  }

  // =====================================================
  // BUILD
  // =====================================================

  @override
  Widget build(
    BuildContext context,
  ) {
    if (isLoading) {
      return Scaffold(
        appBar: AppBar(
          title: const Text(
            'Update Lokasi',
          ),
        ),

        body: const Center(
          child:
              CircularProgressIndicator(),
        ),
      );
    }

    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Update Lokasi',
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
              'Pilih Lokasi',

              style: TextStyle(
                fontSize: 26,

                fontWeight:
                    FontWeight.bold,
              ),
            ),

            const SizedBox(
              height: 8,
            ),

            Text(
              'Pilih lokasi secara bertingkat.',

              style: TextStyle(
                color:
                    Colors.grey.shade600,
              ),
            ),

            const SizedBox(
              height: 24,
            ),

            _buildDropdown(
              label: 'Provinsi',

              value:
                  selectedProvince,

              items: provinces,

              enabled:
                  provinces.isNotEmpty,

              onChanged:
                  _onProvinceChanged,
            ),

            const SizedBox(
              height: 16,
            ),

            _buildDropdown(
              label:
                  'Kabupaten/Kota',

              value:
                  selectedRegency,

              items: regencies,

              enabled:
                  selectedProvince !=
                          null &&
                      regencies.isNotEmpty,

              onChanged:
                  _onRegencyChanged,
            ),

            const SizedBox(
              height: 16,
            ),

            _buildDropdown(
              label:
                  'Kecamatan',

              value:
                  selectedDistrict,

              items: districts,

              enabled:
                  selectedRegency !=
                          null &&
                      districts.isNotEmpty,

              onChanged:
                  _onDistrictChanged,
            ),

            const SizedBox(
              height: 16,
            ),

            _buildDropdown(
              label:
                  'Kelurahan/Desa',

              value:
                  selectedVillage,

              items: villages,

              enabled:
                  selectedDistrict !=
                          null &&
                      villages.isNotEmpty,

              onChanged:
                  _onVillageChanged,
            ),

            const SizedBox(
              height: 24,
            ),

            if (errorMessage.isNotEmpty)
              Container(
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

                  style: TextStyle(
                    color:
                        Colors.red.shade700,
                  ),
                ),
              ),

            const SizedBox(
              height: 16,
            ),

            SizedBox(
              height: 52,

              child:
                  ElevatedButton.icon(
                onPressed:
                    selectedVillage ==
                            null
                        ? null
                        : _saveLocation,

                icon: const Icon(
                  Icons.save,
                ),

                label: const Text(
                  'Simpan Lokasi',
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

    required List<WilayahData> items,

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

      items: items.map(
        (
          WilayahData item,
        ) {
          return DropdownMenuItem<
              WilayahData>(
            value: item,

            child: Text(
              item.name,

              overflow:
                  TextOverflow.ellipsis,
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
}