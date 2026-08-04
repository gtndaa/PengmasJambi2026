import 'package:shared_preferences/shared_preferences.dart';

class LocationStorageService {
  static const String provinceKey = 'selected_province';
  static const String regencyKey = 'selected_regency';
  static const String districtKey = 'selected_district';
  static const String villageKey = 'selected_village';
  static const String adm4Key = 'selected_adm4';

  // =====================================================
  // SIMPAN LOKASI
  // =====================================================

  static Future<void> saveLocation({
    required String province,
    required String regency,
    required String district,
    required String village,
    required String adm4,
  }) async {
    final prefs =
        await SharedPreferences.getInstance();

    await prefs.setString(
      provinceKey,
      province,
    );

    await prefs.setString(
      regencyKey,
      regency,
    );

    await prefs.setString(
      districtKey,
      district,
    );

    await prefs.setString(
      villageKey,
      village,
    );

    await prefs.setString(
      adm4Key,
      adm4,
    );
  }

  // =====================================================
  // AMBIL LOKASI
  // =====================================================

  static Future<Map<String, String>?>
      getLocation() async {
    final prefs =
        await SharedPreferences.getInstance();

    final province =
        prefs.getString(provinceKey);

    final regency =
        prefs.getString(regencyKey);

    final district =
        prefs.getString(districtKey);

    final village =
        prefs.getString(villageKey);

    final adm4 =
        prefs.getString(adm4Key);

    if (province == null ||
        regency == null ||
        district == null ||
        village == null ||
        adm4 == null) {
      return null;
    }

    return {
      'province': province,
      'regency': regency,
      'district': district,
      'village': village,
      'adm4': adm4,
    };
  }
}