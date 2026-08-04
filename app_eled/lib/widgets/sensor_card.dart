import 'package:flutter/material.dart';
class SensorCard extends StatelessWidget {
  final String title;
  final String value;
  final IconData icon;
  final Color color;
  const SensorCard({
    super.key,
    required this.title,
    required this.value,
    required this.icon,
    required this.color,
  });
  @override
  Widget build(BuildContext context) {
    return Card(
      elevation: 6,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(15),
      ),
      child: Padding(
        padding: const EdgeInsets.all(15),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            CircleAvatar(
              radius: 28,
              backgroundColor: color.withOpacity(.15),
              child: Icon(
                icon,
                color: color,
                size: 30,
              ),
            ),
            const SizedBox(height: 15),
            Text(
              title,
              style: const TextStyle(
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              value,
              style: const TextStyle(
                fontSize: 22,
                fontWeight: FontWeight.bold,
              ),
            ),
          ],
        ),
      ),
    );
  }
}