import 'package:flutter/material.dart';

class BMKGScreen extends StatelessWidget {
  const BMKGScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return const Center(
      child: Text(
        "BMKG Weather",
        style: TextStyle(
          fontSize: 24,
          fontWeight: FontWeight.bold,
        ),
      ),
    );
  }
}