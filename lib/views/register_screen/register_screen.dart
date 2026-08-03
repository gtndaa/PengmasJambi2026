import 'package:flutter/material.dart';

import '../../services/api_service.dart';
class RegisterScreen extends StatefulWidget {
  const RegisterScreen({
    super.key,
  });

  @override
  State<RegisterScreen> createState() =>
      _RegisterScreenState();
}

class _RegisterScreenState
    extends State<RegisterScreen> {
  final firstNameController =
      TextEditingController();

  final lastNameController =
      TextEditingController();

  final usernameController =
      TextEditingController();

  final emailController =
      TextEditingController();

  final passwordController =
      TextEditingController();

  final confirmPasswordController =
      TextEditingController();

  final ApiService apiService =
      ApiService();

  bool obscurePassword = true;
  bool obscureConfirmPassword = true;
  bool isLoading = false;

  @override
  void dispose() {
    firstNameController.dispose();
    lastNameController.dispose();
    usernameController.dispose();
    emailController.dispose();
    passwordController.dispose();
    confirmPasswordController.dispose();

    super.dispose();
  }

  Future<void> register() async {
    if (firstNameController.text
            .trim()
            .isEmpty ||
        lastNameController.text
            .trim()
            .isEmpty ||
        usernameController.text
            .trim()
            .isEmpty ||
        emailController.text
            .trim()
            .isEmpty ||
        passwordController.text.isEmpty ||
        confirmPasswordController
            .text
            .isEmpty) {
      _showMessage(
        'Semua field harus diisi',
      );

      return;
    }

    if (!emailController.text
        .contains('@')) {
      _showMessage(
        'Format email tidak valid',
      );

      return;
    }

    if (passwordController.text.length <
        6) {
      _showMessage(
        'Password minimal 6 karakter',
      );

      return;
    }

    if (passwordController.text !=
        confirmPasswordController.text) {
      _showMessage(
        'Konfirmasi password tidak sama',
      );

      return;
    }

    setState(() {
      isLoading = true;
    });

    final result = await apiService.register(
      firstName: firstNameController.text.trim(),
      lastName: lastNameController.text.trim(),
      username: usernameController.text.trim(),
      email: emailController.text.trim(),
      password: passwordController.text,
    );

    if (!mounted) return;

    setState(() {
      isLoading = false;
    });

    if (!result["success"]) {
      _showMessage(result["message"]);
      return;
    }

    ScaffoldMessenger.of(context)
        .showSnackBar(
      const SnackBar(
        content:
            Text('Akun berhasil dibuat'),
      ),
    );

    // Kembali ke Login dan kirim username
    Navigator.pop(
      context,
      usernameController.text.trim(),
    );
  }

  void _showMessage(String message) {
    ScaffoldMessenger.of(context)
        .showSnackBar(
      SnackBar(
        content: Text(message),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title:
            const Text('Buat Akun'),
      ),
      body: SafeArea(
        child: SingleChildScrollView(
          padding:
              const EdgeInsets.all(24),
          child: Column(
            children: [
              const Icon(
                Icons.agriculture,
                size: 80,
              ),

              const SizedBox(height: 16),

              const Text(
                'Daftar App ELED',
                style: TextStyle(
                  fontSize: 28,
                  fontWeight:
                      FontWeight.bold,
                ),
              ),

              const SizedBox(height: 8),

              const Text(
                'Buat akun untuk mulai memantau kondisi lahanmu',
                textAlign:
                    TextAlign.center,
              ),

              const SizedBox(height: 30),

              TextField(
                controller:
                    firstNameController,
                decoration:
                    const InputDecoration(
                  labelText:
                      'Nama Depan',
                  prefixIcon:
                      Icon(Icons.person),
                  border:
                      OutlineInputBorder(),
                ),
              ),

              const SizedBox(height: 16),

              TextField(
                controller:
                    lastNameController,
                decoration:
                    const InputDecoration(
                  labelText:
                      'Nama Belakang',
                  prefixIcon:
                      Icon(Icons.person),
                  border:
                      OutlineInputBorder(),
                ),
              ),

              const SizedBox(height: 16),

              TextField(
                controller:
                    usernameController,
                decoration:
                    const InputDecoration(
                  labelText:
                      'Username',
                  prefixIcon:
                      Icon(
                    Icons.alternate_email,
                  ),
                  border:
                      OutlineInputBorder(),
                ),
              ),

              const SizedBox(height: 16),

              TextField(
                controller:
                    emailController,
                keyboardType:
                    TextInputType
                        .emailAddress,
                decoration:
                    const InputDecoration(
                  labelText: 'Email',
                  prefixIcon:
                      Icon(Icons.email),
                  border:
                      OutlineInputBorder(),
                ),
              ),

              const SizedBox(height: 16),

              TextField(
                controller:
                    passwordController,
                obscureText:
                    obscurePassword,
                decoration:
                    InputDecoration(
                  labelText: 'Password',
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

              const SizedBox(height: 16),

              TextField(
                controller:
                    confirmPasswordController,
                obscureText:
                    obscureConfirmPassword,
                decoration:
                    InputDecoration(
                  labelText:
                      'Konfirmasi Password',
                  prefixIcon:
                      const Icon(
                    Icons.lock_outline,
                  ),
                  border:
                      const OutlineInputBorder(),
                  suffixIcon:
                      IconButton(
                    onPressed: () {
                      setState(() {
                        obscureConfirmPassword =
                            !obscureConfirmPassword;
                      });
                    },
                    icon: Icon(
                      obscureConfirmPassword
                          ? Icons.visibility
                          : Icons
                              .visibility_off,
                    ),
                  ),
                ),
              ),

              const SizedBox(height: 30),

              SizedBox(
                width: double.infinity,
                height: 50,
                child: ElevatedButton(
                  onPressed:
                      isLoading
                          ? null
                          : register,
                  child: isLoading
                      ? const SizedBox(
                          width: 24,
                          height: 24,
                          child:
                              CircularProgressIndicator(
                            strokeWidth: 2,
                          ),
                        )
                      : const Text(
                          'BUAT AKUN',
                        ),
                ),
              ),

              const SizedBox(height: 16),

              TextButton(
                onPressed: () {
                  Navigator.pop(
                    context,
                  );
                },
                child: const Text(
                  'Sudah punya akun? Login',
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}