import 'package:flutter/material.dart';

import 'consts/colors.dart';
import 'views/splash_screen/splash_screen.dart';

void main() {
  runApp(const WeatherMonitoringApp());
}

class WeatherMonitoringApp extends StatelessWidget {
  const WeatherMonitoringApp({super.key});

  @override
  Widget build(BuildContext context) {

    return MaterialApp(

      debugShowCheckedModeBanner: false,

      title: "Weather Monitoring",

      theme: ThemeData(

        useMaterial3: true,

        scaffoldBackgroundColor: AppColors.background,

        colorScheme: ColorScheme.fromSeed(
          seedColor: AppColors.primary,
        ),

      ),

      home: const SplashScreen(),

    );
  }
}