import 'package:flutter/material.dart';

import '../views/dashboard_screen/dashboard_screen.dart';
import '../views/wifi_screen/wifi_screen.dart';
import '../views/bmkg_screen/bmkg_screen.dart';
import '../views/notification_screen/notification_screen.dart';
import '../views/profile_screen/profile_screen.dart';

class NavigationMenu extends StatefulWidget {
  const NavigationMenu({super.key});

  @override
  State<NavigationMenu> createState() => _NavigationMenuState();
}

class _NavigationMenuState extends State<NavigationMenu> {
  int currentIndex = 0;

  final List<Widget> pages = const [
    DashboardScreen(),
    WifiScreen(),
    BMKGScreen(),
    NotificationScreen(),
    ProfileScreen(),
  ];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: pages[currentIndex],

      bottomNavigationBar: BottomNavigationBar(
        currentIndex: currentIndex,

        type: BottomNavigationBarType.fixed,

        selectedItemColor: Colors.blue,

        unselectedItemColor: Colors.grey,

        onTap: (index) {
          setState(() {
            currentIndex = index;
          });
        },

        items: const [
          BottomNavigationBarItem(
            icon: Icon(Icons.home),
            label: "Home",
          ),

          BottomNavigationBarItem(
            icon: Icon(Icons.wifi),
            label: "WiFi",
          ),

          BottomNavigationBarItem(
            icon: Icon(Icons.cloud),
            label: "BMKG",
          ),

          BottomNavigationBarItem(
            icon: Icon(Icons.notifications),
            label: "Alert",
          ),

          BottomNavigationBarItem(
            icon: Icon(Icons.person),
            label: "Profile",
          ),
        ],
      ),
    );
  }
}