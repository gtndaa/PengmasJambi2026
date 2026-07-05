import 'dart:async';

import 'package:flutter/material.dart';

import '../login_screen/login_screen.dart';

class SplashScreen extends StatefulWidget {
  const SplashScreen({super.key});

  @override
  State<SplashScreen> createState() => _SplashScreenState();
}

class _SplashScreenState extends State<SplashScreen> {

  @override
  void initState() {
    super.initState();

    Timer(
      const Duration(seconds: 3),
      () {
        Navigator.pushReplacement(
          context,
          MaterialPageRoute(
            builder: (_) => const LoginScreen(),
          ),
        );
      },
    );
  }

  @override
  Widget build(BuildContext context) {

    return Scaffold(

      body: Center(

        child: Column(

          mainAxisAlignment: MainAxisAlignment.center,

          children: [

            Icon(
              Icons.cloud,
              size: 100,
              color: Colors.blue,
            ),

            SizedBox(height: 20),

            Text(
              "Weather Monitoring",
              style: TextStyle(
                fontSize: 28,
                fontWeight: FontWeight.bold,
              ),
            ),

            SizedBox(height: 10),

            Text(
              "ESP32 • AWS • BMKG",
              style: TextStyle(
                color: Colors.grey,
              ),
            ),

            SizedBox(height: 40),

            CircularProgressIndicator(),
          ],
        ),
      ),
    );
  }
}