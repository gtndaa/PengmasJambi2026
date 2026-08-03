import 'dart:convert';

import 'package:http/http.dart' as http;

import '../models/user_model.dart';

class ApiService {
  // =====================================================
  // BASE URL AWS LAMBDA
  // =====================================================

  static const String baseUrl =
      'https://k27gamn56cmjkns7mcjny4wovu0jbems.lambda-url.ap-southeast-3.on.aws';

  // =====================================================
  // DATA USER YANG SEDANG LOGIN
  // =====================================================

  static UserModel? _currentUser;

  // =====================================================
  // REGISTER
  // =====================================================

  Future<Map<String, dynamic>> register({
    required String firstName,
    required String lastName,
    required String username,
    required String email,
    required String password,
  }) async {
    try {
      final response = await http.post(
        Uri.parse(
          '$baseUrl/register',
        ),
        headers: {
          'Content-Type':
              'application/json',
        },
        body: jsonEncode({
          'firstName':
              firstName.trim(),

          'lastName':
              lastName.trim(),

          'username':
              username.trim(),

          'email':
              email.trim(),

          'password':
              password,

          'punyaAlat':
              false,

          'lokasiLahan':
              '',
        }),
      );

      final data =
          _decodeResponse(
        response.body,
      );

      if (response.statusCode >= 200 &&
          response.statusCode < 300) {
        return {
          'success':
              true,

          'message':
              data['msg'] ??
                  data['message'] ??
                  'Registrasi berhasil',
        };
      }

      return {
        'success':
            false,

        'message':
            data['msg'] ??
                data['message'] ??
                'Registrasi gagal',
      };
    } catch (error) {
      return {
        'success':
            false,

        'message':
            'Gagal terhubung ke server: '
                '$error',
      };
    }
  }

  // =====================================================
  // LOGIN
  // =====================================================

  Future<Map<String, dynamic>> login({
    required String username,
    required String password,
  }) async {
    try {
      final response = await http.post(
        Uri.parse(
          '$baseUrl/login',
        ),
        headers: {
          'Content-Type':
              'application/json',
        },
        body: jsonEncode({
          'username':
              username.trim(),

          'password':
              password,
        }),
      );

      final data =
          _decodeResponse(
        response.body,
      );

      final bool success =
          response.statusCode >= 200 &&
              response.statusCode <
                  300 &&
              data['error'] != true;

      if (!success) {
        return {
          'success':
              false,

          'message':
              data['msg'] ??
                  data['message'] ??
                  'Username atau password salah',
        };
      }

      // Data login dapat berada pada:
      // loginResult, data, atau langsung pada response.

      dynamic userData =
          data['loginResult'] ??
              data['data'] ??
              data;

      if (userData
          is Map<String, dynamic>) {
        _currentUser =
            UserModel.fromJson(
          userData,
        );
      }

      return {
        'success':
            true,

        'data':
            userData,

        'message':
            data['msg'] ??
                data['message'] ??
                'Login berhasil',
      };
    } catch (error) {
      return {
        'success':
            false,

        'message':
            'Gagal terhubung ke server: '
                '$error',
      };
    }
  }

  // =====================================================
  // AMBIL SEMUA USER
  // =====================================================

  Future<List<dynamic>>
      getUsers() async {
    final response =
        await http.get(
      Uri.parse(
        '$baseUrl/users',
      ),
    );

    if (response.statusCode >= 200 &&
        response.statusCode <
            300) {
      final decoded =
          jsonDecode(
        response.body,
      );

      if (decoded is List) {
        return decoded;
      }

      if (decoded
          is Map<String, dynamic>) {
        final users =
            decoded['data'] ??
                decoded['users'];

        if (users is List) {
          return users;
        }
      }

      return [];
    }

    throw Exception(
      'Gagal mengambil data user. '
      'Status: '
      '${response.statusCode}',
    );
  }

  // =====================================================
  // AMBIL USER YANG SEDANG LOGIN
  // =====================================================

  Future<UserModel?>
      getCurrentUser() async {
    return _currentUser;
  }

  // =====================================================
  // CEK STATUS LOGIN
  // =====================================================

  Future<bool>
      isLoggedIn() async {
    return _currentUser != null;
  }

  // =====================================================
  // LOGOUT
  // =====================================================

  Future<void>
      logout() async {
    _currentUser = null;
  }

  // =====================================================
  // KIRIM KONFIGURASI WIFI KE CLOUD
  // =====================================================

  Future<Map<String, dynamic>>
      sendDeviceConfig({
    required Map<String, dynamic>
        config,
  }) async {
    final response =
        await http.post(
      Uri.parse(
        '$baseUrl/config',
      ),
      headers: {
        'Content-Type':
            'application/json',
      },
      body:
          jsonEncode(
        config,
      ),
    );

    print(
      '================================',
    );

    print(
      'DEVICE CONFIG RESPONSE',
    );

    print(
      'STATUS CODE: '
      '${response.statusCode}',
    );

    print(
      'BODY: '
      '${response.body}',
    );

    print(
      '================================',
    );

    final data =
        _decodeResponse(
      response.body,
    );

    if (response.statusCode >= 200 &&
        response.statusCode <
            300) {
      return {
        'success':
            true,

        'message':
            data['msg'] ??
                data['message'] ??
                'Konfigurasi berhasil dikirim',

        'data':
            data,
      };
    }

    throw Exception(
      data['msg'] ??
          data['message'] ??
          'Gagal mengirim konfigurasi. '
              'Status: '
              '${response.statusCode}',
    );
  }

  // =====================================================
  // HELPER UNTUK MEMBACA RESPONSE JSON
  // =====================================================

  Map<String, dynamic>
      _decodeResponse(
    String responseBody,
  ) {
    if (responseBody
        .trim()
        .isEmpty) {
      return {};
    }

    try {
      final decoded =
          jsonDecode(
        responseBody,
      );

      if (decoded
          is Map<String, dynamic>) {
        return decoded;
      }

      return {
        'data':
            decoded,
      };
    } catch (_) {
      return {
        'message':
            responseBody,
      };
    }
  }
}