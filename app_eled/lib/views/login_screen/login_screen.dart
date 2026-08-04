import 'package:flutter/material.dart';

import '../../services/api_service.dart';
import '../../widgets/navigationbar.dart';
import '../register_screen/register_screen.dart';

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

      setState(() {
        isLoading = false;
      });

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
  Widget build(
    BuildContext context,
  ) {
    return Scaffold(
      body: SafeArea(
        child: Center(
          child:
              SingleChildScrollView(
            padding:
                const EdgeInsets.all(
              24,
            ),
            child: Column(
              mainAxisAlignment:
                  MainAxisAlignment
                      .center,
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
                  'Login untuk melanjutkan',
                ),

                const SizedBox(
                  height: 35,
                ),

                TextField(
                  controller:
                      identifierController,
                  decoration:
                      const InputDecoration(
                    labelText:
                        'Username',
                    hintText:
                        'Masukkan username',
                    prefixIcon:
                        Icon(
                      Icons.person,
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
                    prefixIcon:
                        const Icon(
                      Icons.lock,
                    ),
                    border:
                        const OutlineInputBorder(),
                    suffixIcon:
                        IconButton(
                      onPressed: () {
                        setState(() {
                          obscurePassword =
                              !obscurePassword;
                        });
                      },
                      icon: Icon(
                        obscurePassword
                            ? Icons.visibility
                            : Icons
                                .visibility_off,
                      ),
                    ),
                  ),
                ),

                const SizedBox(
                  height: 30,
                ),

                SizedBox(
                  width:
                      double.infinity,
                  height: 50,
                  child:
                      ElevatedButton(
                    onPressed:
                        isLoading
                            ? null
                            : login,
                    child:
                        isLoading
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

                const SizedBox(
                  height: 16,
                ),

                Row(
                  mainAxisAlignment:
                      MainAxisAlignment
                          .center,
                  children: [
                    const Text(
                      'Belum punya akun?',
                    ),

                    TextButton(
                      onPressed:
                          openRegister,
                      child:
                          const Text(
                        'Daftar',
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