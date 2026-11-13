import 'package:shared_preferences/shared_preferences.dart';

import 'api_service.dart';

/// Lokasi pilihan user disimpan PER AKUN, supaya akun baru di HP yang sama
/// tidak mewarisi lokasi akun lain.
///
/// Selain salinan per akun, ada "lokasi aktif" (key tanpa akhiran username)
/// yang dibaca pengecek peringatan BMKG di background. Background isolate
/// tidak tahu siapa yang sedang login, jadi lokasi aktif selalu disamakan
/// dengan lokasi akun yang sedang login dan dikosongkan saat logout.
class LocationStorageService {
  static const String provinceKey = 'selected_province';
  static const String regencyKey = 'selected_regency';
  static const String districtKey = 'selected_district';
  static const String villageKey = 'selected_village';
  static const String adm4Key = 'selected_adm4';

  static const List<String> _fields = [
    provinceKey,
    regencyKey,
    districtKey,
    villageKey,
    adm4Key,
  ];

  static const Map<String, String> _mapKeys = {
    provinceKey: 'province',
    regencyKey: 'regency',
    districtKey: 'district',
    villageKey: 'village',
    adm4Key: 'adm4',
  };

  static String _userKey(String field, String username) =>
      '$field@$username';

  // =====================================================
  // SIMPAN LOKASI (akun yang sedang login)
  // =====================================================

  static Future<void> saveLocation({
    required String province,
    required String regency,
    required String district,
    required String village,
    required String adm4,
  }) async {
    final prefs = await SharedPreferences.getInstance();
    final username = ApiService.currentUsername;

    final values = {
      provinceKey: province,
      regencyKey: regency,
      districtKey: district,
      villageKey: village,
      adm4Key: adm4,
    };

    for (final e in values.entries) {
      if (username != null) {
        await prefs.setString(_userKey(e.key, username), e.value);
      }
      // lokasi aktif untuk pengecek peringatan di background
      await prefs.setString(e.key, e.value);
    }
  }

  // =====================================================
  // AMBIL LOKASI (akun yang sedang login)
  // =====================================================

  static Future<Map<String, String>?> getLocation() async {
    final username = ApiService.currentUsername;
    if (username == null) return null;

    final prefs = await SharedPreferences.getInstance();
    final result = <String, String>{};

    for (final field in _fields) {
      final value = prefs.getString(_userKey(field, username));
      if (value == null) return null;
      result[_mapKeys[field]!] = value;
    }

    return result;
  }

  // =====================================================
  // SAMAKAN LOKASI AKTIF DENGAN AKUN YANG LOGIN
  // =====================================================

  /// Dipanggil setelah login berhasil. Kalau akun ini belum pernah memilih
  /// lokasi, lokasi aktif dikosongkan (tidak memakai lokasi akun sebelumnya).
  static Future<void> activateForCurrentUser() async {
    final prefs = await SharedPreferences.getInstance();
    final location = await getLocation();

    if (location == null) {
      await clearActiveLocation();
      return;
    }

    for (final field in _fields) {
      await prefs.setString(field, location[_mapKeys[field]!]!);
    }
  }

  /// Dipanggil saat logout: berhenti memantau peringatan untuk lokasi akun
  /// yang keluar. Lokasi per akun tetap tersimpan untuk login berikutnya.
  static Future<void> clearActiveLocation() async {
    final prefs = await SharedPreferences.getInstance();
    for (final field in _fields) {
      await prefs.remove(field);
    }
  }
}
