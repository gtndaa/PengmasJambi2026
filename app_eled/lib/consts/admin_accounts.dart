import '../services/api_service.dart';

/// Akun yang boleh membuka WiFi Settings & Advanced Settings.
/// Tulis username dengan huruf kecil.
const Set<String> adminUsernames = {'kyoh', 'gita', 'admin', 'galihmsa'};

/// true kalau akun yang sedang login termasuk [adminUsernames].
bool get isAdminUser {
  final name = ApiService.currentUsername;
  return name != null && adminUsernames.contains(name);
}
