import 'package:flutter/material.dart';

class AppColors {
  static const Color primary = Color(0xFF1565C0);
  static const Color secondary = Color(0xFF42A5F5);

  static const Color background = Color(0xFFF5F7FA);

  static const Color white = Colors.white;

  static const Color black = Colors.black;

  static const Color success = Colors.green;

  static const Color warning = Colors.orange;

  static const Color danger = Colors.red;
  static const Color ink = Color(0xFF1E2A3A);
  static const Color inkSoft = Color(0xFF607080);
  static const List<Color> blueGradient = [Color(0xFF1565C0), Color(0xFF42A5F5)];
  static const List<Color> violetGradient = [Color(0xFF6B6FD6), Color(0xFF9C9EF0)];
    // Text

  // Rain history card
  static const List<Color> rainGradient = [
    Color(0xFF6C7BFF),
    Color(0xFF8E7CFF),
  ];

  // Hero gradient that follows the weather (like your references:
  // blue = clear, violet = storm, teal = rain)
  static List<Color> weatherGradient(String desc) {
    final t = desc.toLowerCase();
    if (t.contains('petir') || t.contains('badai')) {
      return const [Color(0xFF6B6FD6), Color(0xFF9C9EF0)];
    }
    if (t.contains('hujan')) {
      return const [Color(0xFF4F9D8F), Color(0xFF86C2B4)];
    }
    if (t.contains('berawan')) {
      return const [Color(0xFF6C8BE0), Color(0xFF9DB7F0)];
    }
    return const [Color(0xFF3FA9F5), Color(0xFF7CC4FA)]; // cerah / default
  }
}