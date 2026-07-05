import 'package:flutter/material.dart';

import '../../widgets/custom_button.dart';
import '../../widgets/custom_textfield.dart';

class WifiScreen extends StatefulWidget {
  const WifiScreen({super.key});

  @override
  State<WifiScreen> createState() => _WifiScreenState();
}

class _WifiScreenState extends State<WifiScreen> {

  final ssidController = TextEditingController();

  final passwordController = TextEditingController();

  final deviceController = TextEditingController();

  final intervalController = TextEditingController();

  bool autoReconnect = true;

  @override
  Widget build(BuildContext context) {

    return Scaffold(

      appBar: AppBar(

        title: const Text("WiFi Configuration"),

      ),

      body: SingleChildScrollView(

        padding: const EdgeInsets.all(20),

        child: Column(

          crossAxisAlignment: CrossAxisAlignment.start,

          children: [

            const Text(

              "WiFi SSID",

              style: TextStyle(
                fontWeight: FontWeight.bold,
              ),

            ),

            const SizedBox(height: 8),

            CustomTextField(

              hint: "SSID",

              icon: Icons.wifi,

              controller: ssidController,

            ),

            const SizedBox(height: 20),

            const Text(

              "Password",

              style: TextStyle(
                fontWeight: FontWeight.bold,
              ),

            ),

            const SizedBox(height: 8),

            CustomTextField(

              hint: "Password",

              icon: Icons.lock,

              obscure: true,

              controller: passwordController,

            ),

            const SizedBox(height: 20),

            const Text(

              "Device Name",

              style: TextStyle(
                fontWeight: FontWeight.bold,
              ),

            ),

            const SizedBox(height: 8),

            CustomTextField(

              hint: "ESP32 Weather Station",

              icon: Icons.memory,

              controller: deviceController,

            ),

            const SizedBox(height: 20),

            const Text(

              "Upload Interval (seconds)",

              style: TextStyle(
                fontWeight: FontWeight.bold,
              ),

            ),

            const SizedBox(height: 8),

            CustomTextField(

              hint: "30",

              icon: Icons.timer,

              controller: intervalController,

            ),

            const SizedBox(height: 20),

            SwitchListTile(

              title: const Text("Auto Reconnect"),

              value: autoReconnect,

              onChanged: (value){

                setState(() {

                  autoReconnect = value;

                });

              },

            ),

            const SizedBox(height: 20),

            CustomButton(

              text: "Save Configuration",

              onPressed: (){

                ScaffoldMessenger.of(context).showSnackBar(

                  const SnackBar(

                    content: Text("Configuration Saved"),

                  ),

                );

              },

            ),
            const SizedBox(height: 15),
            CustomButton(
              text: "Send to ESP32",
              onPressed: (){
                ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(
                    content: Text("Configuration Sent"),
                  ),
                );
              },
            ),
          ],
        ),
      ),
    );
  }
}