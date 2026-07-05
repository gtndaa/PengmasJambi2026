import 'package:flutter/material.dart';

import '../../widgets/sensor_card.dart';
import '../../widgets/status_card.dart';

class DashboardScreen extends StatelessWidget {

  const DashboardScreen({super.key});

  @override
  Widget build(BuildContext context) {

    return Scaffold(

      appBar: AppBar(

        backgroundColor: Colors.blue,

        foregroundColor: Colors.white,

        elevation: 0,

        title: const Text("Weather Monitoring"),

      ),

      body: SingleChildScrollView(

        child: Padding(

          padding: const EdgeInsets.all(15),

          child: Column(

            crossAxisAlignment: CrossAxisAlignment.start,

            children: [

              Card(

                elevation: 5,

                child: Padding(

                  padding: const EdgeInsets.all(20),

                  child: Column(

                    crossAxisAlignment: CrossAxisAlignment.start,

                    children: const [

                      Text(
                        "Device",
                        style: TextStyle(
                          fontWeight: FontWeight.bold,
                          fontSize: 20,
                        ),
                      ),

                      SizedBox(height: 8),

                      Text("ESP32 Weather Station"),

                      SizedBox(height: 5),

                      Text("Location : Bandung"),

                      SizedBox(height: 5),

                      Text("Last Update : 09:45 AM"),

                    ],

                  ),

                ),

              ),

              const SizedBox(height: 20),

              const Text(

                "Sensor Data",

                style: TextStyle(

                  fontWeight: FontWeight.bold,

                  fontSize: 22,

                ),

              ),

              const SizedBox(height: 15),

              GridView.count(

                physics: const NeverScrollableScrollPhysics(),

                shrinkWrap: true,

                crossAxisCount: 2,

                crossAxisSpacing: 12,

                mainAxisSpacing: 12,

                childAspectRatio: 1,

                children: const [

                  SensorCard(

                    title: "Temperature",

                    value: "29°C",

                    icon: Icons.thermostat,

                    color: Colors.red,

                  ),

                  SensorCard(

                    title: "Humidity",

                    value: "82%",

                    icon: Icons.water_drop,

                    color: Colors.blue,

                  ),

                  SensorCard(

                    title: "Pressure",

                    value: "1008 hPa",

                    icon: Icons.speed,

                    color: Colors.orange,

                  ),

                  SensorCard(

                    title: "Rainfall",

                    value: "0 mm",

                    icon: Icons.cloud,

                    color: Colors.indigo,

                  ),

                  SensorCard(

                    title: "Wind",

                    value: "12 km/h",

                    icon: Icons.air,

                    color: Colors.green,

                  ),

                  SensorCard(

                    title: "Battery",

                    value: "95%",

                    icon: Icons.battery_full,

                    color: Colors.green,

                  ),

                ],

              ),

              const SizedBox(height: 25),

              const Text(

                "Connection Status",

                style: TextStyle(

                  fontWeight: FontWeight.bold,

                  fontSize: 22,

                ),

              ),

              const SizedBox(height: 10),

              const StatusCard(

                title: "WiFi",

                status: true,

              ),

              const StatusCard(

                title: "AWS IoT",

                status: true,

              ),

            ],

          ),

        ),

      ),

    );

  }

}