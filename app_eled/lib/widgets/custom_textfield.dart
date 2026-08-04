import 'package:flutter/material.dart';

class CustomTextField extends StatelessWidget {

  final String hint;
  final IconData icon;
  final bool obscure;
  final TextEditingController controller;

  const CustomTextField({
    super.key,
    required this.hint,
    required this.icon,
    required this.controller,
    this.obscure = false,
  });

  @override
  Widget build(BuildContext context) {

    return TextField(

      controller: controller,

      obscureText: obscure,

      decoration: InputDecoration(

        prefixIcon: Icon(icon),

        hintText: hint,

        border: OutlineInputBorder(

          borderRadius: BorderRadius.circular(12),

        ),
      ),
    );
  }
}