import 'package:flutter/material.dart';

class StatusCard extends StatelessWidget {

  final String title;
  final bool status;

  const StatusCard({
    super.key,
    required this.title,
    required this.status,
  });

  @override
  Widget build(BuildContext context) {

    return Card(

      elevation: 4,

      child: ListTile(

        leading: Icon(
          status ? Icons.check_circle : Icons.cancel,
          color: status ? Colors.green : Colors.red,
        ),

        title: Text(title),

        subtitle: Text(
          status ? "Connected" : "Disconnected",
        ),

      ),

    );

  }

}