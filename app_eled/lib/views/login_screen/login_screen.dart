import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../../models/user_model.dart';
import '../../services/api_service.dart';
import '../../widgets/navigationbar.dart';
import '../register_screen/register_screen.dart';
import '../../consts/colors.dart';
import '../../widgets/gradient_header.dart';

class LoginScreen extends StatefulWidget {
  const LoginScreen({
    super.key,
  });

  @override
  State<LoginScreen> createState() =>
      _LoginScreenState();
}

class _LoginScreenState
    extends State<LoginScreen> {
  final identifierController =
      TextEditingController();

  final passwordController =
      TextEditingController();

  final ApiService apiService =
      ApiService();

  bool isLoading = false;
  bool obscurePassword = true;

  @override
  void dispose() {
    identifierController.dispose();
    passwordController.dispose();

    super.dispose();
  }

  Future<void> login() async {
    final username =
        identifierController.text.trim();

    final password =
        passwordController.text;

    if (username.isEmpty ||
        password.isEmpty) {
      _showMessage(
        'Username dan password wajib diisi',
      );

      return;
    }

    setState(() {
      isLoading = true;
    });

    try {
      // ==========================================
      // LOGIN KE API
      // ==========================================

      final result =
          await apiService.login(
        username: username,
        password: password,
      );

      if (!mounted) {
        return;
      }

      if (!result['success']) {
        setState(() {
          isLoading = false;
        });

        _showMessage(
          result['message'] ??
              'Login gagal',
        );

        return;
      }

      // ==========================================
      // AMBIL NAMA DEPAN USER
      // ==========================================

      final prefs =
          await SharedPreferences.getInstance();

      final loginIdentifier =
          username.trim().toLowerCase();

      try {
        final users =
            await apiService.getUsers();

        UserModel? matchedUser;

        for (final item in users) {
          if (item is Map) {
            final userMap =
                Map<String, dynamic>.from(
              item,
            );

            final candidate =
                UserModel.fromJson(
              userMap,
            );

            final candidateUsername =
                candidate.username
                    .trim()
                    .toLowerCase();

            final candidateEmail =
                candidate.email
                    .trim()
                    .toLowerCase();

            if (candidateUsername ==
                    loginIdentifier ||
                candidateEmail ==
                    loginIdentifier) {
              matchedUser =
                  candidate;

              break;
            }
          }
        }

        if (matchedUser != null &&
            matchedUser.firstName
                .trim()
                .isNotEmpty) {
          final firstName =
              matchedUser.firstName
                  .trim();

          await prefs.setString(
            'user_name',
            firstName,
          );

          debugPrint(
            'NAMA USER DITEMUKAN: '
            '$firstName',
          );
        } else {
          await prefs.remove(
            'user_name',
          );

          debugPrint(
            'Nama user tidak ditemukan',
          );
        }
      } catch (e) {
        debugPrint(
          'Gagal mengambil nama user: $e',
        );
      }

      if (!mounted) {
        return;
      }

      setState(() {
        isLoading = false;
      });

      // ==========================================
      // MASUK KE APLIKASI
      // ==========================================

      Navigator.pushReplacement(
        context,
        MaterialPageRoute(
          builder: (_) =>
              const NavigationMenu(),
        ),
      );
    } catch (error) {
      if (!mounted) {
        return;
      }

      setState(() {
        isLoading = false;
      });

      _showMessage(
        'Terjadi kesalahan: $error',
      );
    }
  }

  Future<void> openRegister() async {
    final registeredUsername =
        await Navigator.push<String>(
      context,
      MaterialPageRoute(
        builder: (_) =>
            const RegisterScreen(),
      ),
    );

    if (registeredUsername != null &&
        mounted) {
      identifierController.text =
          registeredUsername;
    }
  }

  void _showMessage(
    String message,
  ) {
    ScaffoldMessenger.of(context)
        .showSnackBar(
      SnackBar(
        content: Text(
          message,
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppColors.background,
      body: SafeArea(
        child: Center(
          child: SingleChildScrollView(
            padding: const EdgeInsets.all(20),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const GradientHeader(
                  icon: Icons.agriculture,
                  title: 'App ELED',
                  subtitle: 'Login untuk melanjutkan',
                  colors: AppColors.blueGradient,
                ),
                const SizedBox(height: 20),
                Card(
                  child: Padding(
                    padding: const EdgeInsets.all(18),
                    child: Column(
                      children: [
                        TextField(
                          controller: identifierController,
                          decoration: const InputDecoration(
                            labelText: 'Username',
                            hintText: 'Masukkan username',
                            prefixIcon: Icon(Icons.person_outline),
                          ),
                        ),
                        const SizedBox(height: 14),
                        TextField(
                          controller: passwordController,
                          obscureText: obscurePassword,
                          decoration: InputDecoration(
                            labelText: 'Password',
                            prefixIcon: const Icon(Icons.lock_outline),
                            suffixIcon: IconButton(
                              onPressed: () {
                                setState(() {
                                  obscurePassword = !obscurePassword;
                                });
                              },
                              icon: Icon(
                                obscurePassword
                                    ? Icons.visibility_outlined
                                    : Icons.visibility_off_outlined,
                              ),
                            ),
                          ),
                        ),
                        const SizedBox(height: 22),
                        SizedBox(
                          width: double.infinity,
                          height: 50,
                          child: ElevatedButton(
                            onPressed: isLoading ? null : login,
                            style: ElevatedButton.styleFrom(
                              elevation: 0,
                              backgroundColor: AppColors.primary,
                              foregroundColor: Colors.white,
                              shape: RoundedRectangleBorder(
                                borderRadius: BorderRadius.circular(16),
                              ),
                            ),
                            child: isLoading
                                ? const SizedBox(
                                    width: 24,
                                    height: 24,
                                    child: CircularProgressIndicator(
                                      strokeWidth: 2,
                                      color: Colors.white,
                                    ),
                                  )
                                : const Text(
                                    'LOGIN',
                                    style: TextStyle(
                                      fontWeight: FontWeight.w600,
                                      letterSpacing: 0.5,
                                    ),
                                  ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
                const SizedBox(height: 14),
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    const Text('Belum punya akun?'),
                    TextButton(
                      onPressed: openRegister,
                      child: const Text(
                        'Daftar',
                        style: TextStyle(
                          fontWeight: FontWeight.w600,
                          color: AppColors.primary,
                        ),
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}