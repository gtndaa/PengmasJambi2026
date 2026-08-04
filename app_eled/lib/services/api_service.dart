import 'dart:convert';

import 'package:http/http.dart'
    as http;

import '../models/sensor_model.dart';
import '../models/user_model.dart';

class ApiService {
  // =====================================================
  // BASE URL AWS LAMBDA
  // =====================================================

  static const String baseUrl =
      'https://k27gamn56cmjkns7mcjny4wovu0jbems.lambda-url.ap-southeast-3.on.aws';

  // =====================================================
  // USER YANG SEDANG LOGIN
  // =====================================================

  static UserModel? _currentUser;

  // =====================================================
  // REGISTER
  // =====================================================

  Future<Map<String, dynamic>>
      register({
    required String firstName,
    required String lastName,
    required String username,
    required String email,
    required String password,
  }) async {
    try {
      final response =
          await http.post(
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
        }),
      );

      final data =
          _decodeResponse(
        response.body,
      );

      if (response.statusCode >=
              200 &&
          response.statusCode <
              300) {
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
            'Gagal terhubung '
                'ke server: '
                '$error',
      };
    }
  }

  // =====================================================
  // LOGIN
  // =====================================================

  Future<Map<String, dynamic>>
      login({
    required String username,
    required String password,
  }) async {
    try {
      final response =
          await http.post(
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

      final success =
          response.statusCode >=
                  200 &&
              response.statusCode <
                  300 &&
              data['error'] !=
                  true;

      if (!success) {
        return {
          'success':
              false,

          'message':
              data['msg'] ??
                  data['message'] ??
                  'Username atau '
                      'password salah',
        };
      }

      final dynamic userData =
          data['loginResult'] ??
              data['data'] ??
              data;

      print(
        '================================',
      );

      print(
        'LOGIN API RESPONSE',
      );

      print(
        response.body,
      );

      print(
        'USER DATA YANG DIPROSES',
      );

      print(
        userData,
      );

      print(
        '================================',
      );

      if (userData
          is Map<String, dynamic>) {
        _currentUser =
            UserModel.fromJson(
          userData,
        );

        print(
          'FIRST NAME: '
          '${_currentUser?.firstName}',
        );

        print(
          'LAST NAME: '
          '${_currentUser?.lastName}',
        );

        print(
          'FULL NAME: '
          '${_currentUser?.fullName}',
        );

        print(
          'USERNAME: '
          '${_currentUser?.username}',
        );

        print(
          'EMAIL: '
          '${_currentUser?.email}',
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
            'Gagal terhubung '
                'ke server: '
                '$error',
      };
    }
  }

  // =====================================================
  // AMBIL SEMUA USER
  // =====================================================

  Future<List<dynamic>>
      getUsers() async {
    try {
      final response =
          await http.get(
        Uri.parse(
          '$baseUrl/users',
        ),
        headers: {
          'Content-Type':
              'application/json',
        },
      );

      if (response.statusCode >=
              200 &&
          response.statusCode <
              300) {
        if (response.body
            .trim()
            .isEmpty) {
          return [];
        }

        final decoded =
            jsonDecode(
          response.body,
        );

        if (decoded is List) {
          return decoded;
        }

        if (decoded
            is Map<String,
                dynamic>) {
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
        'Gagal mengambil '
        'data user. '
        'Status: '
        '${response.statusCode}',
      );
    } catch (error) {
      throw Exception(
        'Gagal mengambil '
        'data user: '
        '$error',
      );
    }
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
    return _currentUser !=
        null;
  }

  // =====================================================
  // LOGOUT
  // =====================================================

  Future<void>
      logout() async {
    _currentUser =
        null;
  }

  // =====================================================
  // KIRIM KONFIGURASI PERANGKAT
  // =====================================================

  Future<Map<String, dynamic>>
      sendDeviceConfig({
    required String wifiSSID,
    required String wifiPassword,
    required int uploadInterval,
    required int listenWindow,
    required int sleepInterval,
    required bool useDeepSleep,
  }) async {
    try {
      final response =
          await http.post(
        Uri.parse(
          '$baseUrl/config',
        ),
        headers: {
          'Content-Type':
              'application/json',
        },
        body: jsonEncode({
          'wifiSSID':
              wifiSSID.trim(),

          'wifiPassword':
              wifiPassword,

          'uploadInterval':
              uploadInterval,

          'listenWindow':
              listenWindow,

          'sleepInterval':
              sleepInterval,

          'useDeepSleep':
              useDeepSleep,
        }),
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

      final responseData =
          _decodeResponse(
        response.body,
      );

      if (response.statusCode >=
              200 &&
          response.statusCode <
              300) {
        return {
          'success':
              true,

          'message':
              responseData[
                      'message'] ??
                  responseData[
                      'msg'] ??
                  'Konfigurasi '
                      'berhasil dikirim',

          'data':
              responseData,
        };
      }

      return {
        'success':
            false,

        'message':
            responseData[
                    'message'] ??
                responseData[
                    'msg'] ??
                'Server '
                    'mengembalikan '
                    'status '
                    '${response.statusCode}',
      };
    } catch (error) {
      return {
        'success':
            false,

        'message':
            'Gagal mengirim '
                'konfigurasi: '
                '$error',
      };
    }
  }

  // =====================================================
  // AMBIL KONFIGURASI PERANGKAT
  // =====================================================

  Future<Map<String, dynamic>>
      getDeviceConfig() async {
    try {
      final response =
          await http.get(
        Uri.parse(
          '$baseUrl/config',
        ),
        headers: {
          'Content-Type':
              'application/json',
        },
      );

      print(
        '================================',
      );

      print(
        'GET DEVICE CONFIG',
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

      if (response.statusCode <
              200 ||
          response.statusCode >=
              300) {
        throw Exception(
          'Gagal mengambil '
          'konfigurasi. '
          'Status: '
          '${response.statusCode}',
        );
      }

      final decoded =
          _decodeResponse(
        response.body,
      );

      // Format langsung:
      //
      // {
      //   "wifiSSID": "galih",
      //   "uploadInterval": 300000
      // }

      if (decoded.containsKey(
        'uploadInterval',
      )) {
        return decoded;
      }

      // Format:
      //
      // {
      //   "success": true,
      //   "data": {
      //     "uploadInterval": 300000
      //   }
      // }

      final data =
          decoded['data'];

      if (data
          is Map<String,
              dynamic>) {
        return data;
      }

      throw Exception(
        'uploadInterval '
        'tidak ditemukan '
        'pada respons API',
      );
    } catch (error) {
      throw Exception(
        'Gagal membaca '
        'konfigurasi: '
        '$error',
      );
    }
  }

  // =====================================================
  // AMBIL DATA SENSOR DALAM BENTUK MAP
  // =====================================================
  //
  // Fungsi ini digunakan oleh:
  //
  // data_lahan_screen.dart
  //
  // Contoh:
  //
  // final data =
  //     await apiService
  //         .getSensorData();
  //
  // =====================================================

  Future<Map<String, dynamic>>
      getSensorData() async {
    try {
      final response =
          await http.get(
        Uri.parse(
          '$baseUrl/sensordata',
        ),
        headers: {
          'Content-Type':
              'application/json',
        },
      );

      print(
        '================================',
      );

      print(
        'GET SENSOR DATA',
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

      if (response.statusCode <
              200 ||
          response.statusCode >=
              300) {
        throw Exception(
          'Gagal mengambil '
          'data sensor. '
          'Status: '
          '${response.statusCode}',
        );
      }

      if (response.body
          .trim()
          .isEmpty) {
        throw Exception(
          'Respons data sensor '
          'kosong',
        );
      }

      final decoded =
          jsonDecode(
        response.body,
      );

      if (decoded
          is! Map<String,
              dynamic>) {
        throw Exception(
          'Format respons API '
          'tidak valid',
        );
      }

      // Jika API mengirim:
      //
      // {
      //   "success": true,
      //   "data": {
      //     "temp_out": 22.3
      //   }
      // }

      if (decoded[
              'success'] ==
          false) {
        throw Exception(
          decoded[
                  'message'] ??
              decoded[
                  'msg'] ??
              'API mengembalikan '
                  'status gagal',
        );
      }

      final sensorData =
          decoded['data'];

      if (sensorData
          is Map<String,
              dynamic>) {
        return sensorData;
      }

      // Jika API mengirim data
      // langsung tanpa "data".

      if (decoded.containsKey(
        'temp_out',
      )) {
        return decoded;
      }

      throw Exception(
        'Data sensor '
        'tidak ditemukan',
      );
    } catch (error) {
      throw Exception(
        'Gagal membaca '
        'data sensor: '
        '$error',
      );
    }
  }

  // =====================================================
  // AMBIL DATA SENSOR DALAM BENTUK MODEL
  // =====================================================
  //
  // Fungsi ini digunakan jika halaman
  // memakai SensorDataModel.
  //
  // =====================================================

  Future<SensorDataModel>
      getLatestSensorData() async {
    final sensorData =
        await getSensorData();

    return SensorDataModel
        .fromJson(
      sensorData,
    );
  }

  // =====================================================
  // MEMBACA RESPONSE JSON
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
          is Map<String,
              dynamic>) {
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