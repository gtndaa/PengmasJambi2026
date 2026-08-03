import 'dart:convert';
import 'dart:math';

import 'package:crypto/crypto.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../models/user_model.dart';

class AuthResult {
  final bool success;
  final String message;

  const AuthResult({
    required this.success,
    required this.message,
  });
}

class AuthService {
  static const String _usersKey = 'app_eled_users';
  static const String _currentUsernameKey =
      'app_eled_current_username';
  static const String _rememberLoginKey =
      'app_eled_remember_login';

  // =========================
  // REGISTER
  // =========================

  Future<AuthResult> register({
    required String firstName,
    required String lastName,
    required String username,
    required String email,
    required String password,
  }) async {
    final users = await _getUsers();

    final normalizedUsername =
        username.trim().toLowerCase();

    final normalizedEmail =
        email.trim().toLowerCase();

    // Cek username
    final usernameExists = users.any(
      (user) =>
          user.username.toLowerCase() ==
          normalizedUsername,
    );

    if (usernameExists) {
      return const AuthResult(
        success: false,
        message: 'Username sudah digunakan',
      );
    }

    // Cek email
    final emailExists = users.any(
      (user) =>
          user.email.toLowerCase() ==
          normalizedEmail,
    );

    if (emailExists) {
      return const AuthResult(
        success: false,
        message: 'Email sudah terdaftar',
      );
    }

    final salt = _generateSalt();

    final passwordHash =
        _hashPassword(password, salt);

    final newUser = UserModel(
      firstName: firstName.trim(),
      lastName: lastName.trim(),
      username: username.trim(),
      email: normalizedEmail,
      passwordHash: passwordHash,
      passwordSalt: salt,
    );

    users.add(newUser);

    await _saveUsers(users);

    return const AuthResult(
      success: true,
      message: 'Akun berhasil dibuat',
    );
  }

  // =========================
  // LOGIN
  // =========================

  Future<AuthResult> login({
    required String identifier,
    required String password,
    required bool rememberMe,
  }) async {
    final users = await _getUsers();

    final normalizedIdentifier =
        identifier.trim().toLowerCase();

    UserModel? foundUser;

    for (final user in users) {
      final usernameMatches =
          user.username.toLowerCase() ==
              normalizedIdentifier;

      final emailMatches =
          user.email.toLowerCase() ==
              normalizedIdentifier;

      if (usernameMatches || emailMatches) {
        foundUser = user;
        break;
      }
    }

    if (foundUser == null) {
      return const AuthResult(
        success: false,
        message: 'User tidak ditemukan',
      );
    }

    final enteredPasswordHash =
        _hashPassword(
      password,
      foundUser.passwordSalt,
    );

    if (enteredPasswordHash !=
        foundUser.passwordHash) {
      return const AuthResult(
        success: false,
        message: 'Password salah',
      );
    }

    final prefs =
        await SharedPreferences.getInstance();

    // Dibutuhkan agar Profile tahu siapa
    // user yang sedang menggunakan aplikasi.
    await prefs.setString(
      _currentUsernameKey,
      foundUser.username,
    );

    // Kalau true, SplashScreen akan langsung
    // mengarahkan user ke Home pada launch berikutnya.
    await prefs.setBool(
      _rememberLoginKey,
      rememberMe,
    );

    return const AuthResult(
      success: true,
      message: 'Login berhasil',
    );
  }

  // =========================
  // SESSION
  // =========================

  Future<bool> isLoggedIn() async {
    final prefs =
        await SharedPreferences.getInstance();

    final rememberLogin =
        prefs.getBool(_rememberLoginKey) ??
            false;

    final currentUsername =
        prefs.getString(_currentUsernameKey);

    return rememberLogin &&
        currentUsername != null;
  }

  Future<UserModel?> getCurrentUser() async {
    final prefs =
        await SharedPreferences.getInstance();

    final username =
        prefs.getString(_currentUsernameKey);

    if (username == null) {
      return null;
    }

    final users = await _getUsers();

    for (final user in users) {
      if (user.username == username) {
        return user;
      }
    }

    return null;
  }

  Future<void> logout() async {
    final prefs =
        await SharedPreferences.getInstance();

    await prefs.remove(_currentUsernameKey);
    await prefs.setBool(
      _rememberLoginKey,
      false,
    );
  }

  // =========================
  // LOCAL STORAGE
  // =========================

  Future<List<UserModel>> _getUsers() async {
    final prefs =
        await SharedPreferences.getInstance();

    final usersJson =
        prefs.getString(_usersKey);

    if (usersJson == null ||
        usersJson.isEmpty) {
      return [];
    }

    try {
      final List<dynamic> decoded =
          jsonDecode(usersJson);

      return decoded
          .map(
            (item) => UserModel.fromJson(
              Map<String, dynamic>.from(item),
            ),
          )
          .toList();
    } catch (_) {
      return [];
    }
  }

  Future<void> _saveUsers(
    List<UserModel> users,
  ) async {
    final prefs =
        await SharedPreferences.getInstance();

    final data = users
        .map((user) => user.toJson())
        .toList();

    await prefs.setString(
      _usersKey,
      jsonEncode(data),
    );
  }

  // =========================
  // PASSWORD HASH
  // =========================

  String _generateSalt() {
    final random = Random.secure();

    final values = List<int>.generate(
      16,
      (_) => random.nextInt(256),
    );

    return base64UrlEncode(values);
  }

  String _hashPassword(
    String password,
    String salt,
  ) {
    final bytes =
        utf8.encode('$salt$password');

    return sha256
        .convert(bytes)
        .toString();
  }
}