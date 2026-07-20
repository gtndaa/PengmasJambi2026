import 'package:flutter/material.dart';
import 'package:pengmas/widgets/navigationbar.dart';

// Sesuaikan import DashboardScreen dengan struktur folder project Anda
import '../dashboard_screen/dashboard_screen.dart';

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
  final TextEditingController emailController =
      TextEditingController();

  final TextEditingController passwordController =
      TextEditingController();

  bool isLoading = false;

  bool obscurePassword = true;

  @override
  void dispose() {
    emailController.dispose();
    passwordController.dispose();

    super.dispose();
  }

  Future<void> login() async {
    final email =
        emailController.text.trim();

    final password =
        passwordController.text.trim();

    if (email.isEmpty ||
        password.isEmpty) {
      ScaffoldMessenger.of(context)
          .showSnackBar(
        const SnackBar(
          content: Text(
            'Email dan password harus diisi',
          ),
        ),
      );

      return;
    }

    setState(() {
      isLoading = true;
    });

    try {
      /*
       * TEMPORARY LOGIN
       *
       * Untuk sementara, login dianggap
       * berhasil jika email dan password
       * tidak kosong.
       *
       * Proses:
       *
       * Login
       *   ↓
       * Dashboard
       *   ↓
       * BMKG Screen
       *   ↓
       * GPS
       *   ↓
       * ADM4 otomatis
       *   ↓
       * Data BMKG
       */

      await Future.delayed(
        const Duration(
          seconds: 1,
        ),
      );

      if (!mounted) return;

      Navigator.pushReplacement(
        context,
        MaterialPageRoute(
          builder: (context) =>
              const NavigationMenu(),
        ),
      );
    } catch (e) {
      if (!mounted) return;

      ScaffoldMessenger.of(context)
          .showSnackBar(
        SnackBar(
          content: Text(
            'Login gagal: $e',
          ),
        ),
      );
    } finally {
      if (mounted) {
        setState(() {
          isLoading = false;
        });
      }
    }
  }

  @override
  Widget build(
    BuildContext context,
  ) {
    return Scaffold(
      body: SafeArea(
        child: Center(
          child: SingleChildScrollView(
            padding:
                const EdgeInsets.all(24),

            child: Column(
              mainAxisAlignment:
                  MainAxisAlignment.center,

              children: [
                const Icon(
                  Icons.agriculture,
                  size: 90,
                ),

                const SizedBox(
                  height: 20,
                ),

                const Text(
                  'App ELED',
                  style: TextStyle(
                    fontSize: 30,
                    fontWeight:
                        FontWeight.bold,
                  ),
                ),

                const SizedBox(
                  height: 8,
                ),

                const Text(
                  'Silakan login untuk melanjutkan',
                  textAlign:
                      TextAlign.center,
                ),

                const SizedBox(
                  height: 35,
                ),

                TextField(
                  controller:
                      emailController,

                  keyboardType:
                      TextInputType
                          .emailAddress,

                  decoration:
                      const InputDecoration(
                    labelText: 'Email',

                    hintText:
                        'Masukkan email',

                    prefixIcon:
                        Icon(
                      Icons.email,
                    ),

                    border:
                        OutlineInputBorder(),
                  ),
                ),

                const SizedBox(
                  height: 20,
                ),

                TextField(
                  controller:
                      passwordController,

                  obscureText:
                      obscurePassword,

                  decoration:
                      InputDecoration(
                    labelText:
                        'Password',

                    hintText:
                        'Masukkan password',

                    prefixIcon:
                        const Icon(
                      Icons.lock,
                    ),

                    suffixIcon:
                        IconButton(
                      icon: Icon(
                        obscurePassword
                            ? Icons
                                .visibility
                            : Icons
                                .visibility_off,
                      ),

                      onPressed: () {
                        setState(() {
                          obscurePassword =
                              !obscurePassword;
                        });
                      },
                    ),

                    border:
                        const OutlineInputBorder(),
                  ),
                ),

                const SizedBox(
                  height: 30,
                ),

                SizedBox(
                  width:
                      double.infinity,

                  height: 50,

                  child: ElevatedButton(
                    onPressed: isLoading
                        ? null
                        : login,

                    child: isLoading
                        ? const SizedBox(
                            width: 24,

                            height: 24,

                            child:
                                CircularProgressIndicator(
                              strokeWidth:
                                  2,
                            ),
                          )
                        : const Text(
                            'LOGIN',
                          ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}