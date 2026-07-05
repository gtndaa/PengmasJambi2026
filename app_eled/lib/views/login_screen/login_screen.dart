import 'package:flutter/material.dart';

import '../../widgets/custom_button.dart';
import '../../widgets/custom_textfield.dart';
import '../dashboard_screen/dashboard_screen.dart';
import '../../widgets/navigationbar.dart';

class LoginScreen extends StatefulWidget {

  const LoginScreen({super.key});

  @override
  State<LoginScreen> createState() => _LoginScreenState();
}

class _LoginScreenState extends State<LoginScreen> {

  final emailController = TextEditingController();

  final passwordController = TextEditingController();

  @override
  Widget build(BuildContext context) {

    return Scaffold(

      body: SafeArea(

        child: SingleChildScrollView(

          padding: const EdgeInsets.all(25),

          child: Column(

            children: [

              const SizedBox(height: 50),

              const Icon(
                Icons.cloud,
                color: Colors.blue,
                size: 90,
              ),

              const SizedBox(height: 20),

              const Text(
                "Weather Monitoring",
                style: TextStyle(
                  fontSize: 30,
                  fontWeight: FontWeight.bold,
                ),
              ),

              const SizedBox(height: 8),

              const Text(
                "ESP32 • AWS • BMKG",
                style: TextStyle(
                  color: Colors.grey,
                ),
              ),

              const SizedBox(height: 50),

              CustomTextField(

                hint: "Email",

                icon: Icons.email,

                controller: emailController,

              ),

              const SizedBox(height: 20),

              CustomTextField(

                hint: "Password",

                icon: Icons.lock,

                controller: passwordController,

                obscure: true,

              ),

              const SizedBox(height: 35),

              CustomButton(

                text: "LOGIN",

                onPressed: () {

                  Navigator.pushReplacement(
                    context,
                    MaterialPageRoute(
                      builder: (_) => const NavigationMenu(),
                    ),

                  );
                },

              ),

              const SizedBox(height: 25),

              Row(

                mainAxisAlignment: MainAxisAlignment.center,

                children: [

                  const Text(
                    "Don't have an account?",
                  ),

                  TextButton(

                    onPressed: () {

                    },

                    child: const Text(
                      "Register",
                    ),

                  ),

                ],
              )

            ],
          ),
        ),
      ),
    );
  }
}